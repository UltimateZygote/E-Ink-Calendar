#include <Arduino.h>
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_3C.h>

#include <credentials.h>

#include "time.h"
#include <string>
#include <ArduinoJson.h>              // https://github.com/bblanchon/ArduinoJson needs version v6 or above
#include "Wire.h"

#include <WiFi.h>
#include <WebServer.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h> // Needs to be from the ESP32 platform version 3.2.0 or later, as the previous has problems with http-redirect (google script)

const char *ssid = "SAM FLORES";          // your network SSID (name of wifi network)
const char *password = "legoland";  // your network password

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT/2> display(GxEPD2_750c_Z08(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

HTTPClient http;

bool displayCalendar();
void displayCalendarEntry(String eventTime, String eventName, String eventDesc);

struct calendarEntries
{
  String calTime;
  String calTitle;
};

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  //Initialize e-ink display
  display.init(115200); // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(13, 12, 14, 15); // Map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15) - adjusted to the recommended PIN settings from Waveshare - note that this is not the default for most screens
  //SPI.begin(18, 19, 23, 5); //

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  calendarRequest = "https://script.google.com/macros/s/" + googleAPI + "/exec";
  displayCalendar();
}

void loop() {
  // put your main code here, to run repeatedly:

}


bool displayCalendar()
{

  // Getting calendar from your published google script
  Serial.println("Getting calendar");
  Serial.println(calendarRequest);

  http.end();
  http.setTimeout(20000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(calendarRequest)) {
    Serial.println("Cannot connect to google script");
    return false;
  } 

  Serial.println("Connected to google script");
  int returnCode = http.GET();
  Serial.print("Returncode: "); Serial.println(returnCode);
  String response = http.getString();
  Serial.print("Response: "); Serial.println(response);

  int indexFrom = 0;
  int indexTo = 0;
  int cutTo = 0;

  String strBuffer = "";

  int count = 0;
  int line = 0;
  struct calendarEntries calEnt[calEntryCount];

  Serial.println("IntexFrom");  
  indexFrom = response.lastIndexOf("\n") + 1;



  // Fill calendarEntries with entries from the get-request
  while (indexTo>=0 && line<calEntryCount) {
    count++;
    indexTo = response.indexOf(";",indexFrom);
    cutTo = indexTo;

    if(indexTo != -1) { 
      strBuffer = response.substring(indexFrom, cutTo);
      
      indexFrom = indexTo + 1;
      
      Serial.println(strBuffer);

      if(count == 1) {
        // Set entry time
        calEnt[line].calTime = strBuffer.substring(0,21); //Exclude end date and time to avoid clutter - Format is "Wed Feb 10 2020 10:00"

      } else if(count == 2) {
        // Set entry title
        calEnt[line].calTitle = strBuffer;

      } else {
          count = 0;
          line++;
      }
    }
  }

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }


  // All data is now gathered and processed.
  // Prepare to refresh the display with the calendar entries and weather info

  // Turn off text-wrapping
  display.setTextWrap(false);

  display.setRotation(calendarOrientation);

  // Clear the screen with white using full window mode. Not strictly nessecary, but as I selected to use partial window for the content, I decided to do a full refresh first.
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  }   while(display.nextPage());

  // Print the content on the screen - I use a partial window refresh for the entire width and height, as I find this makes a clearer picture
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    int x = calendarPosX;
    int y = calendarPosY;

    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    // Print mini-test in top in white (e.g. not visible) - avoids a graphical glitch I observed in all first-lines printed
    display.setCursor(x, 0);
    display.setTextColor(GxEPD_WHITE);
    display.setFont(fontEntryTime);
    display.print(weekday[timeinfo.tm_wday]);

    // Print morning greeting (Happy X-day)
    display.setCursor(x, y);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(fontMainTitle);
    display.print(weekday[timeinfo.tm_wday]);


    // Set position for the first calendar entry
    y = y + 45;
    
    // Print calendar entries from first [0] to the last fetched [line-1] - in case there is fewer events than the maximum allowed
    for(int i=0;  i < line; i++) {

      // Print event time
      display.setCursor(x, y);
      display.setFont(fontEntryTime);
      display.print(calEnt[i].calTime);

      // Print event title
      display.setCursor(x, y + 30);
      display.setFont(fontEntryTitle);
      display.print(calEnt[i].calTitle);

      // Prepare y-position for next event entry
      y = y + calendarSpacing;

    }

  } while(display.nextPage());

  return true;
}