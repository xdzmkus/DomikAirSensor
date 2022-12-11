//#define DEBUG_DRAW

// *** TFT-1.4 *** 128 x 128 ***//

#include <Adafruit_ST7735.h>

#define TFT_RST   -1
#define TFT_CS    D4
#define TFT_DC    D3

// OPTION 1 (recommended) is to use the HARDWARE SPI pins, which are unique
// to each board and not reassignable. For Wemos D1 mini:
// MOSI = pin D7 and SCLK = pin D5.
// This is the fastest mode of operation and is required if
// using the breakout board's microSD card.
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// OPTION 2 lets you interface the display using ANY TWO or THREE PINS,
// tradeoff being that performance is not as fast as hardware SPI above.
//#define TFT_MOSI 11  // Data out
//#define TFT_SCLK 13  // Clock out
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

unsigned int background = ST7735_BLACK;
unsigned int color = ST7735_WHITE;

#include "GFX_Text.h"
#include "GFX_FloatEx.h"
#include "GFX_IntegerEx.h"
#include "GFX_Boolean.h"

#define TEMP_TREND_X 5
#define TEMP_TREND_Y 0
#define TEMP_TREND_W 28
#define TEMP_TREND_H 40

GFX_Boolean lcdTempTrend = GFX_Boolean(true, String(static_cast<char>(24)) /*↑*/, String(static_cast<char>(25)) /*↓*/, 4, &tft, background, color, TEMP_TREND_X, TEMP_TREND_Y, TEMP_TREND_W, TEMP_TREND_H);

#define TEMP_X 28
#define TEMP_Y 0
#define TEMP_W 100
#define TEMP_H 40

GFX_FloatEx lcdTemp = GFX_FloatEx(0.0F, 2, 4, String(static_cast<char>(248)) /*°*/ + "C", 2, &tft, background, color, TEMP_X, TEMP_Y, TEMP_W, TEMP_H);

#define CO2_X 5
#define CO2_Y 40
#define CO2_W 0
#define CO2_H 48

GFX_IntegerEx lcdCO2 = GFX_IntegerEx(0, 5, "ppm", 1, &tft, background, color, CO2_X, CO2_Y, CO2_W, CO2_H);

#define HUM_TREND_X 5
#define HUM_TREND_Y 88
#define HUM_TREND_W 28
#define HUM_TREND_H 40

GFX_Boolean lcdHumTrend = GFX_Boolean(true, String(static_cast<char>(24)) /*↑*/, String(static_cast<char>(25)) /*↓*/, 4, &tft, background, color, HUM_TREND_X, HUM_TREND_Y, HUM_TREND_W, HUM_TREND_H);

#define HUM_X 28
#define HUM_Y 88
#define HUM_W 100
#define HUM_H 40

GFX_FloatEx lcdHum = GFX_FloatEx(0.0F, 1, 4, "%", 2, &tft, background, color, HUM_X, HUM_Y, HUM_W, HUM_H);


void setup_TFT()
{
    SerialDebug::log(LOG_LEVEL::INFO, F("LCD init..."));

    // TFT init
    tft.initR(INITR_144GREENTAB);	// Init ST7735R chip, green tab
    tft.cp437(true);
    tft.setRotation(0);
    tft.setTextWrap(false);			// Allow text to run off right edge
    tft.fillScreen(background);

    lcdTempTrend.show();

    lcdTemp.setDecimalSize(2);
    lcdTemp.show();

    lcdCO2.show();

    lcdHumTrend.show();

    lcdHum.setDecimalSize(2);
    lcdHum.show();
}

void updateWeather(float temp, float hum, float pres, float alt)
{
    static float avgT = 0;
    static float avgH = 0;

    lcdTempTrend.updateValue(temp > avgT);
    avgT = (avgT + temp) / 2;

//    SerialDebug::log(LOG_LEVEL::DEBUG, String(F("Avg Temperature: ")) + avgT);

    lcdTemp.updateValue(temp);

    lcdHumTrend.updateValue(hum > avgH);
    avgH = (avgH + hum) / 2;

//    SerialDebug::log(LOG_LEVEL::DEBUG, String(F("Avg Humudity: ")) + avgH);

    lcdHum.updateValue(hum);
}

void updateCO2(unsigned int CO2)
{
    lcdCO2.updateValue(CO2);

    if (CO2 < 500 && background != ST7735_BLUE)
    {
        setColors(ST7735_BLUE, ST7735_WHITE);
    }
    else if (CO2 >= 500 && CO2 < 800 && background != ST7735_GREEN)
    {
        setColors(ST7735_GREEN, ST7735_BLACK);
    }
    else if (CO2 >= 800 && CO2 < 1000 && background != ST7735_ORANGE)
    {
        setColors(ST7735_ORANGE, ST7735_BLACK);
    }
    else if (CO2 >= 1000 && CO2 < 2500 && background != ST7735_RED)
    {
        setColors(ST7735_RED, ST7735_WHITE);
    }
    else if (CO2 >= 2500 && background != ST7735_BLACK)
    {
        setColors(ST7735_BLACK, ST7735_WHITE);
    }
}

void setColors(unsigned int bg, unsigned int fg)
{
    background = bg;
    color = fg;

    lcdTempTrend.setColors(background, color);
    lcdTemp.setColors(background, color);
    lcdCO2.setColors(background, color);
    lcdHumTrend.setColors(background, color);
    lcdHum.setColors(background, color);

    tft.fillScreen(background);

    lcdTempTrend.show();
    lcdTemp.show();
    lcdCO2.show();
    lcdHumTrend.show();
    lcdHum.show();
}

