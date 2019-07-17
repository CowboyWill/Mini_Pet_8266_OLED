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
 *   NTPClient.h         - Customized NTPClient library
 *   ESP8266WiFi.h       - WiFi library
 *   WiFiUdp.h           - UDP library for NTP 
 *   SH1106Wire.h        - OLED display library
 *   ArduinoJson.h       - JSON converting library
 *   ArduinoOTA.h        - Over the Air update
 *   Ticker.h            - Timer
 *   OLEDDisplayUi.h     - UI lib
 *   images.h            - custom images
 *   WeatherStationFonts - Font icons
 ****************************************************************************/
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SH1106Wire.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include "OLEDDisplayUi.h"
#include "ESP8266_OLED.h"
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

int counter = 360;

//String result;
//boolean night = false;
//String weatherDescription ="";
//String weatherLocation = "";
//float Temperature;

//extern  unsigned char  cloud[];
//extern  unsigned char  thunder[];
//extern  unsigned char  wind[];


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

/***************NOT WORKING ****************
AutoConnectConfig  Config;
Config.hostName = hostname;
Portal.config(Config);
*******************************************/

// This array keeps function pointers to all frames
// frames are the single panels that slide in 
//FrameCallback frames[] = { drawWeatherNow, drawWeatherToday, digitalClockFrame, drawWeatherTomorrow, drawForecast, analogClockFrame };
FrameCallback frames[] = { digitalClockFrame, analogClockFrame };
int numberOfFrames = 2;

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
  ArduinoOTA.setHostname((const char *)hostname.c_str());
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

}

void loop() {

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    timeClient.update();
    Portal.handleClient();
    ArduinoOTA.handle();

    if(counter == 360) { //Get new data every 30 minutes
      counter = 0;
  //    getWeatherData();
    } else {
      counter++;
    }
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
  //! **********************************************************************
  //String temp = wunderground.getCurrentTemp() + "°F";
  String temp = "100°F";
  display->drawString(100, 54, temp);  // 95

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
  //String weatherIcon = wunderground.getTodayIcon();
  String weatherIcon = "B";
  display->drawString(75, 55, weatherIcon);  // 64,69,    74

  // Horizontal line above Overlay
  display->drawHorizontalLine(0, 52, 128);
 
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



/*



void getWeatherData() //client function to send/receive GET request data.
{
  Serial.println("Getting Weather Data");
  if (client.connect(servername, 80)) {  //starts client connection, checks for connection
    client.println("GET /data/2.5/forecast?id="+CityID+"&units=imperial&cnt=1&APPID="+APIKEY);
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  } 
  else {
    Serial.println("connection failed"); //error message if no client connect
    Serial.println();
  }

  while(client.connected() && !client.available()) delay(1); //waits for data
 
  Serial.println("Waiting for data");

  while (client.connected() || client.available()) { //connected or data available
    char c = client.read(); //gets byte from ethernet buffer
      result = result+c;
  }

  client.stop(); //stop client
  result.replace('[', ' ');
  result.replace(']', ' ');
  Serial.println(result);

  char jsonArray [result.length()+1];
  result.toCharArray(jsonArray,sizeof(jsonArray));
  jsonArray[result.length() + 1] = '\0';

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc,jsonArray);
  if (error) {
    Serial.print("deserializeJson() failed with code ");
    Serial.println(error.c_str());
  }

  String location = doc["city"]["name"];
  String temperature = doc["list"]["main"]["temp"];
  String weather = doc["list"]["weather"]["main"];
  String description = doc["list"]["weather"]["description"];
  String idString = doc["list"]["weather"]["id"];
  String timeS = doc["list"]["dt_txt"];

  timeS = convertGMTTimeToLocal(timeS);

  int length = temperature.length();
  if(length==5) {
    temperature.remove(length-1);
  }

  Serial.println(location);
  Serial.println(weather);
  Serial.println(temperature);
  Serial.println(description);
  Serial.println(temperature);
  Serial.println(timeS);

  clearScreen();

  int weatherID = idString.toInt();
  printData(timeS,temperature, timeS, weatherID);

}

void printData(String timeString, String temperature, String time, int weatherID)
{
  u8g2.drawStr(35, 10, timeString);
  printWeatherIcon(weatherID);
  u8g2.drawStr(27, 60, temperature);
  u8g2.drawStr(83,60,"o");
  u8g2.drawStr(93,132("F");
}

void printWeatherIcon(int id)
{
 switch(id)
 {
  case 800: drawClearWeather(); break;
  case 801: drawFewClouds(); break;
  case 802: drawFewClouds(); break;
  case 803: drawCloud(); break;
  case 804: drawCloud(); break;
  
  case 200: drawThunderstorm(); break;
  case 201: drawThunderstorm(); break;
  case 202: drawThunderstorm(); break;
  case 210: drawThunderstorm(); break;
  case 211: drawThunderstorm(); break;
  case 212: drawThunderstorm(); break;
  case 221: drawThunderstorm(); break;
  case 230: drawThunderstorm(); break;
  case 231: drawThunderstorm(); break;
  case 232: drawThunderstorm(); break;

  case 300: drawLightRain(); break;
  case 301: drawLightRain(); break;
  case 302: drawLightRain(); break;
  case 310: drawLightRain(); break;
  case 311: drawLightRain(); break;
  case 312: drawLightRain(); break;
  case 313: drawLightRain(); break;
  case 314: drawLightRain(); break;
  case 321: drawLightRain(); break;

  case 500: drawLightRainWithSunOrMoon(); break;
  case 501: drawLightRainWithSunOrMoon(); break;
  case 502: drawLightRainWithSunOrMoon(); break;
  case 503: drawLightRainWithSunOrMoon(); break;
  case 504: drawLightRainWithSunOrMoon(); break;
  case 511: drawLightRain(); break;
  case 520: drawModerateRain(); break;
  case 521: drawModerateRain(); break;
  case 522: drawHeavyRain(); break;
  case 531: drawHeavyRain(); break;

  case 600: drawLightSnowfall(); break;
  case 601: drawModerateSnowfall(); break;
  case 602: drawHeavySnowfall(); break;
  case 611: drawLightSnowfall(); break;
  case 612: drawLightSnowfall(); break;
  case 615: drawLightSnowfall(); break;
  case 616: drawLightSnowfall(); break;
  case 620: drawLightSnowfall(); break;
  case 621: drawModerateSnowfall(); break;
  case 622: drawHeavySnowfall(); break;

  case 701: drawFog(); break;
  case 711: drawFog(); break;
  case 721: drawFog(); break;
  case 731: drawFog(); break;
  case 741: drawFog(); break;
  case 751: drawFog(); break;
  case 761: drawFog(); break;
  case 762: drawFog(); break;
  case 771: drawFog(); break;
  case 781: drawFog(); break;

  default:break; 
 }
}

String convertGMTTimeToLocal(String timeS) {
 int length = timeS.length();
 timeS = timeS.substring(length-8,length-6);
 int time = timeS.toInt();
 if(TimeZone >=0) {
    time = time+TimeZone;
 }else {
    time = time+TimeZone;
    if(time<0) {
      time = 24-time;
    }
 }

 if(time > 21 ||  time<7) {
  night=true;
 }else {
  night = false;
 }
 timeS = String(time)+":00";
 return timeS;
}


void clearScreen() {
    tft.fillScreen(BLACK);
}

void drawClearWeather() {
  if(night) {
    drawTheMoon();
  }else {
    drawTheSun();
  }
}

void drawFewClouds() {
  if(night)  {
    drawCloudAndTheMoon();
  }else {
    drawCloudWithSun();
  }
}

void drawTheSun() {
    tft.fillCircle(64,80,26,YELLOW);
}

void drawTheFullMoon() {
    tft.fillCircle(64,80,26,GREY);
}

void drawTheMoon() {
    tft.fillCircle(64,80,26,GREY);
    tft.fillCircle(75,73,26,BLACK);
}

void drawCloud() {
     tft.drawBitmap(0,35,cloud,128,90,GREY);
}

void drawCloudWithSun() {
     tft.fillCircle(73,70,20,YELLOW);
     tft.drawBitmap(0,36,cloud,128,90,BLACK);
     tft.drawBitmap(0,40,cloud,128,90,GREY);
}

void drawLightRainWithSunOrMoon() {
  if(night) {
    drawCloudTheMoonAndRain();
  }else {
    drawCloudSunAndRain();
  }
}

void drawLightRain()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawModerateRain()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
}

void drawHeavyRain()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(43, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
     tft.fillRoundRect(87, 102, 3, 15, 1, BLUE);
}

void drawThunderstorm()
{
     tft.drawBitmap(0,40,thunder,128,90,YELLOW);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(48, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(55, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(74, 102, 3, 15, 1, BLUE);
     tft.fillRoundRect(82, 102, 3, 15, 1, BLUE);
}

void drawLightSnowfall()
{
     tft.drawBitmap(0,30,cloud,128,90,GREY);
     tft.fillCircle(50, 100, 3, GREY);
     tft.fillCircle(65, 103, 3, GREY);
     tft.fillCircle(82, 100, 3, GREY);
}

void drawModerateSnowfall()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillCircle(50, 105, 3, GREY);
     tft.fillCircle(50, 115, 3, GREY);
     tft.fillCircle(65, 108, 3, GREY);
     tft.fillCircle(65, 118, 3, GREY);
     tft.fillCircle(82, 105, 3, GREY);
     tft.fillCircle(82, 115, 3, GREY);
}

void drawHeavySnowfall()
{
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillCircle(40, 105, 3, GREY);
     tft.fillCircle(52, 105, 3, GREY);
     tft.fillCircle(52, 115, 3, GREY);
     tft.fillCircle(65, 108, 3, GREY);
     tft.fillCircle(65, 118, 3, GREY);
     tft.fillCircle(80, 105, 3, GREY);
     tft.fillCircle(80, 115, 3, GREY);
     tft.fillCircle(92, 105, 3, GREY);     
}

void drawCloudSunAndRain()
{
     tft.fillCircle(73,70,20,YELLOW);
     tft.drawBitmap(0,32,cloud,128,90,BLACK);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawCloudAndTheMoon()
{
     tft.fillCircle(94,60,18,GREY);
     tft.fillCircle(105,53,18,BLACK);
     tft.drawBitmap(0,32,cloud,128,90,BLACK);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
}

void drawCloudTheMoonAndRain()
{
     tft.fillCircle(94,60,18,GREY);
     tft.fillCircle(105,53,18,BLACK);
     tft.drawBitmap(0,32,cloud,128,90,BLACK);
     tft.drawBitmap(0,35,cloud,128,90,GREY);
     tft.fillRoundRect(50, 105, 3, 11, 1, BLUE);
     tft.fillRoundRect(65, 105, 3, 11, 1, BLUE);
     tft.fillRoundRect(80, 105, 3, 11, 1, BLUE);
}

void drawWind()
{  
     tft.drawBitmap(0,35,wind,128,90,GREY);   
}

void drawFog()
{
  tft.fillRoundRect(45, 60, 40, 4, 1, GREY);
  tft.fillRoundRect(40, 70, 50, 4, 1, GREY);
  tft.fillRoundRect(35, 80, 60, 4, 1, GREY);
  tft.fillRoundRect(40, 90, 50, 4, 1, GREY);
  tft.fillRoundRect(45, 100, 40, 4, 1, GREY);
}

void clearIcon()
{
     tft.fillRect(0,40,128,100,BLACK);
}



 */
