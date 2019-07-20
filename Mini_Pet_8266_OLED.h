/****************************************************************************
 * @file Mini_Pet_8266_OLED.h
 * @author Will Travis
 * @date Jul 2019
 * @brief Header file with settings for Mini_Pet_8266_OLED 
 *
 * The first section is user settable settings
 * Other settings should not be changed unless absolutly needed
 * @see http://
 ****************************************************************************/

/* User settings */

const String HOSTNAME = "COMMODORE-PET-" + String(ESP.getChipId(), HEX); /* hostname of computer */
const int UPDATE_INTERVAL_SECS = 10 * 60; /* Update every 30 minutes (changed to 10 temporarily) */
const char WEATHER_SERVER_NAME[]="api.openweathermap.org";  // weather site for API calls
const String WEATHER_API_KEY = "336b11533f0125e3a2088546d0a95fad";
//  Search https://openweathermap.org/find for a location. Select the entry closest to the 
//    actual location you want to display data for. 
const String WEATHER_CITY_ID = "5037649"; //Minneapolis, MN USA
const String WEATHER_LANGUAGE = "en" ;  // English
const bool WEATHER_METRIC = false;  // Imperial results
const long TIMEZONE = -5; //GMT -5 (CDT)

/* GLOBAL VARIABLES
 * Only change if absolutly needed 
 */

const int SCREEN_W = 128;                 /* Width of screen */
const int SCREEN_H = 64;                  /* Height of screen */
const int CENTER_X = SCREEN_W/2;          /* Width center of screen */
const int CENTER_Y = ((SCREEN_H-16)/2);   /* Height center of screen */
const int CLOCK_RADIUS = 23;              /* Radius of clock display */
const char DAYS_OF_THE_WEEK[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char DAYS_OF_THE_WEEK_SHORT[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


/****************************************************************************
 * Declaring Prototypes
 ****************************************************************************/
String twoDigits(int digits);
void rootPage();
void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
byte getWifiQuality();
void setReadyForWeatherUpdate();


