/****************************************************************************

  File: Mini_Pet_8266_OLED.ino 

  Program: Mini Pet with ESP8266 (WeMos) and SH1160 128x64 OLED display
 
  Author:
    Will Travis
 
  Date:
    07/05/2019
 
  Version: 
    0.2
  
  See Also:
    See instruction file for hookup information.
  
  Documentation:
    Natural Docs (www.naturaldocs.org)
  
  Description:
    .  UPDATE THIS

  Docs:
    Documentation created by NaturalDocs.org.  Documentation style follows
    Arduino standard, https://www.arduino.cc/en/Reference/StyleGuide:

    - Variable names start with a lower-case letter, use uppercase letters as separators and do not use underbars ('_').
    - Function names start with a lower-case letter, use uppercase letters as separators and do not use underbars ('_').
    - Constant names use all capital letters and use underbars ('_') as separators.
    - Using byte, int, unsigned int, etc. vs. int8_t, uint8_t, etc.
    - Avoiding the use of defines as much as possible.
    - Using const instead of define.

  ToDo:
    - OTA Update - Not working
    - Weather
    - Overlay
    - Overlay - WiFi strength
    - 
  COMPLETED:
    - WiFi AutoConnect
    - Animation of display screens
    - NTP time
 *****************************************************************************/

/****************************************************************************
 * Files: Libraries and Include files
 *   ESP8266WiFi.h       - WiFi library
 *   WiFiUdp.h           - UDP library for NTP 
 *   SH1106Wire.h        - OLED display library https://github.com/ThingPulse/esp8266-oled-ssd1306
 *   ArduinoJson.h       - JSON converting library
 *   ArduinoOTA.h        - Over the Air update
 *   Ticker.h            - Timer
 *   NTPClient.h         - Customized NTPClient library
 *   OLEDDisplayUi.h     - UI lib
 *   Mini_Pet_8266_OLED.h- Settings for this app
 *   images.h            - custom images
 *   fonts               - Font icons
 *   OpenWeatherMapCurrent.h  - Current weather with OpenWeatherMap API
 *   OpenWeatherMapForecast.h - Weather forecast with OpenWeatherMap API
 ****************************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SH1106Wire.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include "NTPClient.h"
#include "OLEDDisplayUi.h"
#include "Mini_Pet_8266_OLED.h"
#include "images.h"
#include "fonts.h"

/****************************************************************************
 * Variables: Global Variables
 *   ssid                - WiFi SSID
 *   ssid_password       - WiFi SSID password
 *! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 ****************************************************************************/
//const char* ssid = "Tardis";
//const char* ssid_password = ")P9o8i7u^Y";

// flag changed in the ticker function every UPDATE_INTERVAL_SECS
bool readyForWeatherUpdate = false;
String lastUpdate = "--";

/****************************************************************************
 * Variables: Setup Library instances
 *   client              - WiFi client
 *   display             - OLED instance
 *   ui                  - UI interface
 *   ticker              - timer
 ****************************************************************************/
WiFiClient client;
SH1106Wire display(0x3c, SDA, SCL);  // ADDRESS, SDA, SCL
OLEDDisplayUi ui ( &display );
Ticker ticker;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", TIMEZONE*60*60);  // offset in seconds

ESP8266WebServer Server; 
AutoConnect Portal(Server);

// Current Weather 
OpenWeatherMapCurrent currentWeatherClient;
OpenWeatherMapCurrentData currentWeather;

/***************NOT WORKING ****************
AutoConnectConfig  Config;
Config.hostName = hostname;
Portal.config(Config);
*******************************************/

// This array keeps function pointers to all frames
// frames are the single panels that slide in 
//FrameCallback frames[] = { drawWeatherNow, drawWeatherToday, digitalClockFrame, drawWeatherTomorrow, drawForecast, analogClockFrame };
FrameCallback frames[] = { digitalClockFrame, drawCurrentWeather, analogClockFrame };
int numberOfFrames = 3;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;


/*****************************************************************************
// ! Sample text
*COMMODORE BASIC*
7167 BYTES FREE
READY
LOAD 'TIME',8
RUN 
   03-17-2019
    03:04:48  
 *****************************************************************************/

/*****************************************************************************
 * Function: setup
 * Initial setup 
 *****************************************************************************/
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();


  // initialize display
  display.init();
  display.clear();
  display.display();
  display.flipScreenVertically();

  // Connect to Wi-Fi
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(CENTER_X, 0, "Connecting to");
  display.drawXbm(34, 20, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display.display();
  
  Server.on("/", rootPage);
  if (Portal.begin()) {
    /*
    WiFi.begin(ssid, ssid_password);
    Serial.println("wi fi setup");
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    */
    Serial.println("WiFi connected: " + WiFi.localIP().toString());

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(CENTER_X, 0, "WiFi connected");
    display.drawString(CENTER_X, 20, WiFi.localIP().toString());  
    // ****************************************************************display.drawString(CENTER_X, 40, ssid);
    display.display();

  }
  
  // Setup OTA
  // Serial.println("Hostname: " + hostname);
  ArduinoOTA.setHostname((const char *)HOSTNAME.c_str());
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth()/2, display.getHeight()/2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(display.getWidth()/2, display.getHeight()/2, "Restart");
    display.display();
  });

  timeClient.begin();  // Start NTP client

  delay(1000);  // show screen for 10 seconds


	// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

    //Hack until disableIndicator works:
  //Set an empty symbol
  ui.setActiveSymbol(emptySymbol);
  ui.setInactiveSymbol(emptySymbol);
// Indicator not used, overlay at bottom is used instead
	ui.disableIndicator();

  /*
  // Customize the active and inactive symbol
  ui.setActiveSymbol(active_Symbol);
  ui.setInactiveSymbol(inactive_Symbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(TOP);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);
  */

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, numberOfFrames);

  // Add overlays
  ui.setOverlays(overlays, numberOfOverlays);

  // Initialise the UI
  ui.init();
  display.flipScreenVertically();

  currentWeatherClient.setLanguage(WEATHER_LANGUAGE);
  currentWeatherClient.setMetric(WEATHER_METRIC);
  updateData(&display);

  ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);

}

void loop() {


  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    timeClient.update();
    Portal.handleClient();
    ArduinoOTA.handle();
    delay(remainingTimeBudget);
  }

}


/*****************************************************************************
 * Function: twoDigits
 * Prints leading 0 for digital clock display
 *****************************************************************************/

String twoDigits(int digits){
  if(digits < 10)
    return '0'+String(digits);
  else
    return String(digits);
}

/****************************************************************************
 * Function: main web page
 ****************************************************************************/
void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}

/****************************************************************************
 * Function: displayan Analog Clock
 ****************************************************************************/
void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // ui.disableIndicator();
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();

  // Draw the clock face
  display->drawCircle(CENTER_X + x, CENTER_Y + y, 2);
  //
  //hour ticks
  for( int z=0; z < 360;z= z + 30 ){
  //Begin at 0° and stop at 360°
    float angle = z ;
    angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
    int x2 = ( CENTER_X + ( sin(angle) * CLOCK_RADIUS ) );
    int y2 = ( CENTER_Y - ( cos(angle) * CLOCK_RADIUS ) );
    int x3 = ( CENTER_X + ( sin(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 8 ) ) ) );
    int y3 = ( CENTER_Y - ( cos(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 8 ) ) ) );
    display->drawLine( x2 + x , y2 + y , x3 + x , y3 + y);
  }

  // display second hand
  float angle = s * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  int x3 = ( CENTER_X + ( sin(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 5 ) ) ) );
  int y3 = ( CENTER_Y - ( cos(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 5 ) ) ) );
  display->drawLine( CENTER_X + x , CENTER_Y + y , x3 + x , y3 + y);
  //
  // display minute hand
  angle = m * 6 ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( CENTER_X + ( sin(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 4 ) ) ) );
  y3 = ( CENTER_Y - ( cos(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 4 ) ) ) );
  display->drawLine( CENTER_X + x , CENTER_Y + y , x3 + x , y3 + y);
  //
  // display hour hand
  angle = h * 30 + int( ( m / 12 ) * 6 )   ;
  angle = ( angle / 57.29577951 ) ; //Convert degrees to radians
  x3 = ( CENTER_X + ( sin(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 2 ) ) ) );
  y3 = ( CENTER_Y - ( cos(angle) * ( CLOCK_RADIUS - ( CLOCK_RADIUS / 2 ) ) ) );
  display->drawLine( CENTER_X + x , CENTER_Y + y , x3 + x , y3 + y);
}

/****************************************************************************
 * Function: display a Digital Clock
 ****************************************************************************/
void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
/*
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(CENTER_X + x, 0+y, timeClient.getFormattedTime(1));
  display->drawString(CENTER_X + x, CENTER_Y-8+y, DAYSOFTHEWEEK[timeClient.getDay()]);
  display->drawString(CENTER_X + x, CENTER_Y+10+y, timeClient.getFormattedDate(1));
 */

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = DAYS_OF_THE_WEEK_SHORT[timeClient.getDay()];
  date += "  " + timeClient.getFormattedDate(1);
  //int textWidth = display->getStringWidth(date);
  display->drawString(64 + x, 5 + y, date);

  display->setFont(ArialMT_Plain_24);
  String time = timeClient.getFormattedTime(1);
  //textWidth = display->getStringWidth(time);
  display->drawString(64 + x, 15 + y, time);

}

/****************************************************************************
 * Function: Overlay header
 ****************************************************************************/
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setFont(ArialMT_Plain_10);
  
  // Frame number / total Frames
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 54, String(state->currentFrame + 1) + "/" + String(numberOfFrames));

  // Time
  String time = timeClient.getFormattedTime(1).substring(0, 5);
  time += timeClient.getPM() ? "PM" : "AM";
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(42, 54, time);  //40

  // Current Temp
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  String temp = String(currentWeather.temp, 0) + (WEATHER_METRIC ? "°C" : "°F");
  display->drawString(100, 54, temp);

  // WiFi Signal strength
  byte quality = getWifiQuality();
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        display->setPixel(120 + 2 * i, 63 - j);
      }
    }
  }

  // Weather Icon
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(Meteocons_Plain_10);
  //! **********************************************************************
  // icon text up to 25 characters   https://developer.accuweather.com/weather-icons
  display->drawString(75, 55, currentWeather.iconMeteoCon);  // 64,69,    74

  // Horizontal line above Overlay
  display->drawHorizontalLine(0, 52, 128);
 
}


void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display) {
  drawProgress(display, 25, "Updating time...");
  //timeClient.updateTime();
  delay (500);  //! temp

  drawProgress(display, 50, "Updating conditions...");
  currentWeatherClient.updateCurrentById(&currentWeather, WEATHER_API_KEY, WEATHER_CITY_ID);

  drawProgress(display, 75, "Updating forecasts...");
  //OpenWeatherMapCurrent.updateForecast(API_KEY, LANGUAGE, CITY_ID);
  delay (500);  //! temp

  lastUpdate = timeClient.getFormattedTime(1);
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...");
  delay(1000);
}

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(ArialMT_Plain_10);
  //display->setTextAlignment(TEXT_ALIGN_LEFT);
  //display->drawString(60 + x, 5 + y, currentWeather.description); 
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, currentWeather.description);

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = String(currentWeather.temp, 1) + (WEATHER_METRIC ? "°C" : "°F");
  //display->drawString(60 + x, 15 + y, temp);
  display->drawString(60 + x, 5 + y, temp);

  display->setFont(Meteocons_Plain_42);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(32 + x, 0 + y, currentWeather.iconMeteoCon);

  //String weatherIcon = currentWeather.iconMeteoCon.c_str();
  //int weatherIconWidth = display->getStringWidth(weatherIcon);
  //display->drawString(32 + x - weatherIconWidth / 2, 5 + y, weatherIcon);
}


// converts the dBm to a range between 0 and 100%
byte getWifiQuality() {
  //long dbm = WiFi.RSSI();
  long dbm = random(-120, 1);
  if(dbm <= -100) {
      return 0;
  } else if(dbm >= -50) {
      return 100;
  } else {
      return 2 * (dbm + 100);
  }
}

void setReadyForWeatherUpdate() {
  //Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}

