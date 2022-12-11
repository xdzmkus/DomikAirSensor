#ifndef _WIFIMQTT_H_
#define _WIFIMQTT_H_

#include <WiFiManager.h>

#include <WiFiClient.h>

#include <PubSubClient.h>

#include <ArduinoJson.h>

#include "SerialDebug.h"

#include "my_data_sensitive.h"

#if !defined(WLAN_SSID) || !defined(WLAN_PASS) || !defined(WLAN_HOSTNAME) || !defined(MQTT_SERVER) || !defined(MQTT_SERVERPORT) || !defined(MQTT_USERNAME) || !defined(MQTT_KEY)
#pragma message "Default credentials are used"
#endif

#ifndef WLAN_SSID
#define WLAN_SSID           "AP wifi name"
#endif

#ifndef WLAN_PASS
#define WLAN_PASS           "and password"
#endif

#ifndef WLAN_HOSTNAME
#define WLAN_HOSTNAME       "set hostname"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER         "127.0.0.1"
#endif

#ifndef MQTT_SERVERPORT
#define MQTT_SERVERPORT     1883
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME       "your mqtt username"
#endif

#ifndef MQTT_KEY
#define MQTT_KEY            "and password"
#endif

class WifiMQTT
{
private:

	const char* const DEVICE_manufacturer = "DOMIK";
	const char* const DEVICE_model = "Air Sensor";
	const char* const DEVICE_name = "Domik Air sensor";
	const char* const DEVICE_sw_version = "1.0.0";

	const char* const DEVICE_UNIQUE_ID = "DOMIK-36c";

	const char* const TEMPERATURE_SENSOR_UNIQUE_ID	= "DOMIK-36c-temperature";
	const char* const HUMIDITY_SENSOR_UNIQUE_ID		= "DOMIK-36c-humidity";
	const char* const PRESSURE_SENSOR_UNIQUE_ID		= "DOMIK-36c-pressure";
	const char* const CO2_SENSOR_UNIQUE_ID			= "DOMIK-36c-co2";

	const char* const DISCOVERY_TOPIC_TEMPERATURE	= "homeassistant/sensor/domik-air-sensor/temperature/config";
	const char* const DISCOVERY_TOPIC_HUMIDITY		= "homeassistant/sensor/domik-air-sensor/humidity/config";
	const char* const DISCOVERY_TOPIC_PRESSURE		= "homeassistant/sensor/domik-air-sensor/pressure/config";
	const char* const DISCOVERY_TOPIC_CO2			= "homeassistant/sensor/domik-air-sensor/co2/config";

	const char* const COMMON_STATE_TOPIC			= "homeassistant/sensor/domik-air-sensor/state";
	const char* const AVAIL_STATUS_TOPIC			= "homeassistant/sensor/domik-air-sensor/status";

protected:

	WiFiClient client;

	PubSubClient mqtt;

public:

	static const uint16_t BOOT_TIMEOUT = 5000;

	WifiMQTT();

	void init(bool reconfigure = false);

	void process();

	void publishState(int co2, float temperature, float humidity, float pressure);

protected:

	void discover();

	bool publishJson(const char* const topic, JsonDocument &payload, bool retain = false);

};


WifiMQTT::WifiMQTT()
{
	mqtt.setClient(client);
	mqtt.setServer(MQTT_SERVER, MQTT_SERVERPORT);
	mqtt.setKeepAlive(60);
	mqtt.setBufferSize(1024);
}

void WifiMQTT::init(bool reconfigure)
{
	WiFiManager wm;

	if (reconfigure)
	{
		wm.setConfigPortalTimeout(180);
		wm.setConfigPortalBlocking(true);
		wm.startConfigPortal(WLAN_SSID, WLAN_PASS);

		SerialDebug::log(LOG_LEVEL::INFO, F("WiFi Reconfigured! Rebooting..."));

		delay(BOOT_TIMEOUT);
		ESP.restart();
	}

	// Set the ESP8266 to be a WiFi-client
	WiFi.mode(WIFI_STA);
	WiFi.hostname(WLAN_HOSTNAME);

	SerialDebug::log(LOG_LEVEL::INFO, WLAN_HOSTNAME);

	wm.setEnableConfigPortal(false);

	if (!wm.autoConnect(WLAN_SSID, WLAN_PASS))
	{
		SerialDebug::log(LOG_LEVEL::ERROR, F("Connection Failed! Rebooting..."));
		delay(BOOT_TIMEOUT);
		ESP.restart();
	}

	SerialDebug::log(LOG_LEVEL::INFO, F("WiFi connected"));
	SerialDebug::log(LOG_LEVEL::INFO, String(F("IP address: ")) + WiFi.localIP().toString());
}

void WifiMQTT::process()
{
	if (!mqtt.connected())
	{
		SerialDebug::log(LOG_LEVEL::INFO, F("Connecting to MQTT... "));

		if (!mqtt.connect(DEVICE_UNIQUE_ID, MQTT_USERNAME, MQTT_KEY, AVAIL_STATUS_TOPIC, 0, true, "offline"))
		{
			SerialDebug::log(LOG_LEVEL::ERROR, String(F("Connect error: ")) + mqtt.state());
			SerialDebug::log(LOG_LEVEL::INFO, F("Retry MQTT connection ..."));
			mqtt.disconnect();
			return;
		}
		else
		{
			SerialDebug::log(LOG_LEVEL::INFO, F("MQTT Connected!"));
			
			mqtt.publish(AVAIL_STATUS_TOPIC, "online", true);

			discover();
		}
	}

	mqtt.loop();
}

void WifiMQTT::publishState(int co2, float temperature, float humidity, float pressure)
{
	DynamicJsonDocument doc(128);

	doc["temperature"] = String(temperature, 1);
	doc["humidity"] = String(humidity, 1);
	doc["pressure"] = String(pressure, 1);
	doc["co2"] = co2;

	publishJson(COMMON_STATE_TOPIC, doc);
}

void WifiMQTT::discover()
{
	StaticJsonDocument<512> deviceDoc;

	deviceDoc["platform"] = "mqtt";
	deviceDoc["enabled_by_default"] = true;
	deviceDoc["state_class"] = "measurement";
	deviceDoc["state_topic"] = COMMON_STATE_TOPIC;
	deviceDoc["json_attributes_topic"] = COMMON_STATE_TOPIC;
	deviceDoc["availability"][0]["topic"] = AVAIL_STATUS_TOPIC;

	JsonObject device = deviceDoc.createNestedObject("device");
	device["identifiers"] = DEVICE_UNIQUE_ID;
	device["manufacturer"] = DEVICE_manufacturer;
	device["model"] = DEVICE_model;
	device["name"] = DEVICE_name;
	device["sw_version"] = DEVICE_sw_version;

	DynamicJsonDocument doc(1024);

	doc = deviceDoc;
	doc["name"] = "Temperature";
	doc["device_class"] = "temperature";
	doc["unit_of_measurement"] = "°C";
	doc["value_template"] = "{{ value_json.temperature }}";
	doc["unique_id"] = TEMPERATURE_SENSOR_UNIQUE_ID;

	publishJson(DISCOVERY_TOPIC_TEMPERATURE, doc, true);

	doc = deviceDoc;
	doc["name"] = "Humidity";
	doc["device_class"] = "humidity";
	doc["unit_of_measurement"] = "%";
	doc["value_template"] = "{{ value_json.humidity }}";
	doc["unique_id"] = HUMIDITY_SENSOR_UNIQUE_ID;

	publishJson(DISCOVERY_TOPIC_HUMIDITY, doc, true);

	doc = deviceDoc;
	doc["name"] = "Pressure";
	doc["device_class"] = "pressure";
	doc["unit_of_measurement"] = "hPa";
	doc["value_template"] = "{{ value_json.pressure }}";
	doc["unique_id"] = PRESSURE_SENSOR_UNIQUE_ID;

	publishJson(DISCOVERY_TOPIC_PRESSURE, doc, true);

	doc = deviceDoc;
	doc["name"] = "CO2";
	doc["device_class"] = "carbon_dioxide";
	doc["unit_of_measurement"] = "ppm";
	doc["value_template"] = "{{ value_json.co2 }}";
	doc["unique_id"] = CO2_SENSOR_UNIQUE_ID;

	publishJson(DISCOVERY_TOPIC_CO2, doc, true);
}

bool WifiMQTT::publishJson(const char* const topic, JsonDocument& payload, bool retain)
{
	String jsonString;

	serializeJson(payload, jsonString);

	SerialDebug::log(LOG_LEVEL::DEBUG, String(F("Publish message: ")));
	SerialDebug::log(LOG_LEVEL::DEBUG, jsonString);

	if (!mqtt.publish(topic, jsonString.c_str(), retain))
	{
		SerialDebug::log(LOG_LEVEL::ERROR, String(F("Publish failed: ")) + mqtt.state());

		return false;
	}

	return true;
}

#endif

