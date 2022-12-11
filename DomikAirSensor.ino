
#define SERIAL_DEBUG
#include "SerialDebug.h"

#include "WifiMQTT.h"
WifiMQTT wifiMqtt;

#define BTN_PIN D0

#include <ArduinoDebounceButton.h>
ArduinoDebounceButton btn(BTN_PIN, BUTTON_CONNECTED::VCC, BUTTON_NORMAL::OPEN);

// MHZ-19 - CO2 sensor
#define ANALOGPIN A0
#define RX_PIN D6                                          // D6 - Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN D8                                          // D8 - Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Native to the sensor (do not change)

#include <MHZ19.h>
#include <SoftwareSerial.h>
MHZ19 myMHZ19;
SoftwareSerial mySerial(RX_PIN, TX_PIN);
unsigned int CO2 = 0;

// I2C modules:
// D1 - SCL; 
// D2 - SDA

// Tiny RTC I2C Module
#include <RTClib.h>
RTC_DS1307 rtc;
DateTime displayedTime;

#define BME280
#define SEALEVELPRESSURE_HPA (1013.25)

#ifdef BME280

// BME280 - humidity, temperature & pressure sensor 
#include <Adafruit_BME280.h>

Adafruit_BME280 sensor;

#else
// BMP280 - temperature & pressure sensor 
#include <Adafruit_BMP280.h>

Adafruit_BMP280 sensor;

#endif


float temperature = 0;
float humidity = 0;
float pressure = 0;
float altitude = 0;


#include <Ticker.h>
Ticker tickerSensor;
Ticker tickerCO2;
Ticker tickerRTC;
Ticker tickerMQTT;

// Interrupts ---------------------------/

#define SENSOR_CHECK_INTERVAL	5.0F    // 5 seconds
#define CO2_CHECK_INTERVAL		60.0F   // 60 seconds
#define RTC_CHECK_INTERVAL		500     // 500 ms
#define MQTT_PUBLISH_INTERVAL	30.0F   // 30 seconds

volatile bool checkSensor = false;
volatile bool checkCO2 = false;
volatile bool checkRTC = false;
volatile bool doPublish = false;

void callback_checkSensor()
{
	checkSensor = true;
}

void callback_checkCO2()
{
	checkCO2 = true;
}

void callback_checkRTC()
{
	checkRTC = true;
}

void callback_publishMQTT()
{
	doPublish = true;
}

void handleButtonEvent(const DebounceButton* button, BUTTON_EVENT eventType)
{
	switch (eventType)
	{
	case BUTTON_EVENT::Clicked:
		SerialDebug::log(LOG_LEVEL::DEBUG, F("Clicked"));
		SerialDebug::log(LOG_LEVEL::INFO, F("Publish state..."));
		doPublish = true;
		break;
	case BUTTON_EVENT::LongPressed:
		SerialDebug::log(LOG_LEVEL::DEBUG, F("LongPressed"));
		SerialDebug::log(LOG_LEVEL::WARN, F("Rebooting..."));
		ESP.restart();
		break;
	default:
		break;
	}
}

// -------------------------------------------------------------------------------------------------------------

bool setup_RTC()
{
	SerialDebug::log(LOG_LEVEL::INFO, F("RTC init..."));

	if (!rtc.begin())
	{
		SerialDebug::log(LOG_LEVEL::ERROR, F("Couldn't find RTC"));
		return false;
	}

	if (!rtc.isrunning())
	{
		SerialDebug::log(LOG_LEVEL::WARN, F("RTC is NOT running!"));
		// following line sets the RTC to the date & time this sketch was compiled
		rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}

	return true;
}

bool setup_Sensor()
{
#ifdef BME280
	SerialDebug::log(LOG_LEVEL::INFO, F("BME280 init..."));
#else
	SerialDebug::log(LOG_LEVEL::INFO, F("BMP280 init..."));
#endif

	unsigned status = sensor.begin(0x76);

	if (!status)
	{
		SerialDebug::log(LOG_LEVEL::ERROR, F("Could not find a valid sensor, check wiring, address, sensor ID!"));
		SerialDebug::log(LOG_LEVEL::INFO, String(F("SensorID was: 0x")) + String(sensor.sensorID(), 16));
		SerialDebug::log(LOG_LEVEL::INFO, F("   ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n"));
		SerialDebug::log(LOG_LEVEL::INFO, F("   ID of 0x56-0x58 represents a BMP 280,\n"));
		SerialDebug::log(LOG_LEVEL::INFO, F("   ID of 0x60 represents a BME 280.\n"));
		SerialDebug::log(LOG_LEVEL::INFO, F("   ID of 0x61 represents a BME 680.\n"));
		return false;
	}

	return true;
}

bool setup_MHZ19()
{
	SerialDebug::log(LOG_LEVEL::INFO, F("MHZ-19 init..."));

	mySerial.begin(BAUDRATE);

	myMHZ19.begin(mySerial);                                // Important, Pass your Stream reference 
	myMHZ19.autoCalibration(false);                         // Turn auto calibration OFF

	return myMHZ19.errorCode;
}

void setup(void)
{
#ifdef SERIAL_DEBUG
	Serial.begin(115200);
#endif

	btn.initPin();

	setup_TFT();

	// wait for button to reconfigure WiFi
	delay(wifiMqtt.BOOT_TIMEOUT);

	bool f_setupMode = btn.getCurrentState();

	btn.setEventHandler(handleButtonEvent);

	if (f_setupMode) SerialDebug::log(LOG_LEVEL::WARN, F("BUTTON PRESSED - RECONFIGURE WIFI"));

	wifiMqtt.init(f_setupMode);

	SerialDebug::log(LOG_LEVEL::WARN, String(F("Device restarted")));

	if (!setup_RTC())
	{
		SerialDebug::log(LOG_LEVEL::ERROR, String(F("RTC initialization failed")));
	}
	else
	{
		checkRTC = true;
		tickerRTC.attach_ms(RTC_CHECK_INTERVAL, callback_checkRTC); // 500ms period

		DateTime curTime = displayedTime = rtc.now();

		SerialDebug::log(LOG_LEVEL::DEBUG, String(F("Device time: ")) + curTime.day() + "/" + curTime.month() + "/" + curTime.year() + " " + curTime.hour() + ":" + curTime.minute() + ":" + curTime.second());
	}

	if (!setup_Sensor())
	{
		SerialDebug::log(LOG_LEVEL::ERROR, String(F("Sensor initialization failed")));
	}
	else
	{
		checkSensor = true;
		tickerSensor.attach(SENSOR_CHECK_INTERVAL, callback_checkSensor);
	}

	if (!setup_MHZ19())
	{
		SerialDebug::log(LOG_LEVEL::ERROR, String(F("MHZ19 initialization failed")));
	}
	else
	{
		checkCO2 = true;
		tickerCO2.attach(CO2_CHECK_INTERVAL, callback_checkCO2);
	}

	tickerMQTT.attach(MQTT_PUBLISH_INTERVAL, callback_publishMQTT);
}

void loop()
{
	btn.check();

	wifiMqtt.process();

	if (checkCO2)
	{
		CO2 = myMHZ19.getCO2();    // Request CO2 (as ppm)

		if (CO2 == 0)
		{
			SerialDebug::log(LOG_LEVEL::ERROR, String(F("MHZ19 read failed: ")) + myMHZ19.errorCode);
			myMHZ19.verify();
		}
		else
		{
			checkCO2 = false;

			updateCO2(CO2);     // show new CO2 value
		}
	}

	if (checkRTC)
	{
		checkRTC = false;

		DateTime now = rtc.now();
		if (displayedTime.minute() != now.minute())
		{
			displayedTime = now;
		}
	}

	if (checkSensor)
	{
		checkSensor = false;

#ifdef BME280
		humidity = sensor.readHumidity();
#endif
		temperature = sensor.readTemperature();
		pressure = sensor.readPressure() / 100.0F;
		altitude = sensor.readAltitude(SEALEVELPRESSURE_HPA);

		updateWeather(temperature, humidity, pressure, altitude);
	}

	if (doPublish)
	{
		doPublish = false;
		wifiMqtt.publishState(CO2, temperature, humidity, pressure);
	}
}
