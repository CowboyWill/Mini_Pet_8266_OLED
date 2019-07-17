/****************************************************************************
 * Variables: CONSTANTS
 *   HOSTNAME     - hostname of computer
 *   SCREEN_W     - Width of screen    
 *   SCREEN_H     - Height of screen
 *   CENTER_X     - Width center of screen
 *   CENTER_Y     - Height center of screen
 *   CLOCK_RADIUS - Radius of clock display
 *   UPDATE_INTERVAL_SECS - Number of seconds for updating weather
 *! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 ****************************************************************************/
#define HOSTNAME "COMMODORE-PET-"
String hostname = HOSTNAME + String(ESP.getChipId(), HEX);
const int SCREEN_W = 128;
const int SCREEN_H = 64;
const int CENTER_X = SCREEN_W/2;
const int CENTER_Y = ((SCREEN_H-16)/2);
const int CLOCK_RADIUS = 23;
//const int UPDATE_INTERVAL_SECS = 30 * 60; // Update every 30 minutes
const char SERVER_NAME[]="api.openweathermap.org";  // weather site for API calls
String APIKEY = "336b11533f0125e3a2088546d0a95fad";
String CITY_ID = "5037649"; //Minneapolis, MN USA
const int TIMEZONE = -5; //GMT -5 (CDT)
const char DAYS_OF_THE_WEEK[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char DAYS_OF_THE_WEEK_SHORT[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};



/****************************************************************************
 * Declaring Prototypes: 
 *! !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 ****************************************************************************/
String twoDigits(int digits);
void rootPage();
void analogClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void digitalClockFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
byte getWifiQuality();

