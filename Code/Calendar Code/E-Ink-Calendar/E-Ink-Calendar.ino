#include <Arduino.h>

#include "time.h"
#include <string>
#include <ArduinoJson.h>              // https://github.com/bblanchon/ArduinoJson needs version v6 or above

#include <WiFi.h>
#include <WebServer.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h> // Needs to be from the ESP32 platform version 3.2.0 or later, as the previous has problems with http-redirect (google script)

// display stuff
#define ENABLE_GxEPD2_GFX 0
#include <GxEPD2_3C.h>

GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT/2> display(GxEPD2_750c_Z08(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));

#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>

#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>

/* 
    The following will be configured using the web-portal

    Use your own API key by signing up for a free developer account at https://openweathermap.org/
    Create a script on scripts.google.com to fetch your calendarrequests (as per guide on instructable)
    Find the lattitude and logitude for your location using google maps
*/
String googleAPI = "AKfycbyAJfHsDe0LliZQ_60GWcQWLPeyHKuHmouzIqpzioEDjW1PbJoIZwkluRO-KJLFQS7a";

// Get time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;
const int   daylightOffset_sec = 3600;

// Get calendar
char calendarServer[] = "script.google.com"; 
String calendarRequest = ""; // This will eventually be an URL in the form of "https://script.google.com/macros/s/" + googleAPI + "/exec"

const int calEntryCount = 20;
const int daysShown = 3;

const char *ssid = "SAM FLORES";          // your network SSID (name of wifi network)
const char *password = "legoland";  // your network password

HTTPClient http;

struct calendarEntries
{
  String calStartTime;
  String calEndTime;
  String calTitle;
};

void setup() 
{
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

  display.init(115200); // uses standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(13, 12, 14, 15); // Map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15) - adjusted to the recommended PIN settings from Waveshare - note that this is not the default for most screens
  //SPI.begin(18, 19, 23, 5); //

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  calendarRequest = "https://script.google.com/macros/s/" + googleAPI + "/exec";
  displayCalendar();
}

void loop() 
{
  // nothing
}

bool displayCalendar()
{
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
        // Set entry start time
        calEnt[line].calStartTime = strBuffer.substring(0,21); //Exclude end date and time to avoid clutter - Format is "Wed Feb 10 2020 10:00"

      } else if(count == 2) {
        // Set entry end time
        calEnt[line].calEndTime = strBuffer.substring(0,21); //Exclude end date and time to avoid clutter - Format is "Wed Feb 10 2020 10:00"

      } else if(count == 3) {
        // Set entry title
        calEnt[line].calTitle = strBuffer;

      } else {
        // skip allday events
        if (strBuffer == "false"){
          line++;
        }
        count = 0;
      }
    }
  }

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }

  display.setRotation(3);

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  }   while(display.nextPage());

  int minTime = 8;
  int maxTime = 21;
  for (int i = 0; i < line; i++){
    String startTime = calEnt[i].calStartTime;
    String endTime = calEnt[i].calEndTime;
    if (startTime.substring(16, 18).toInt() * 60 + startTime.substring(19, 21).toInt() < minTime * 60){
      minTime = startTime.substring(16, 18).toInt();
    }

    if (endTime.substring(16, 18).toInt() * 60 + endTime.substring(19, 21).toInt() > maxTime * 60){
      maxTime = endTime.substring(16, 18).toInt();
    }
  }

  const GFXfont *fontDay = &FreeSerif12pt7b;
  const GFXfont *fontTimeLabel = &FreeSerif9pt7b;
  const GFXfont *fontEntryTitle = &FreeSerif9pt7b;

  int timeLabelOffset = 5;
  int horizLineOffset = timeLabelOffset + 5 + 5;

  int dayLabelOffset = 5;
  int vertLineOffset = dayLabelOffset + 5 + 16;

  int vertLineSpacing = (display.width() - horizLineOffset) / daysShown;
  int horizLineSpacing = (display.height() - vertLineOffset) / (maxTime - minTime + 2);

  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    for (int i = horizLineOffset + vertLineSpacing; i < display.width(); i += vertLineSpacing){
      display.drawLine(i, vertLineOffset, i, display.height(), GxEPD_BLACK);
    }

    for (int i = vertLineOffset + horizLineSpacing; i < display.height(); i += horizLineSpacing){
      display.drawLine(vertLineOffset, i, display.width(), i, GxEPD_BLACK);
    }
  } while(display.nextPage());

}

void drawThickRoundRect (int x, int y, int w, int h, int r, uint16_t color, int thick)
{
  for (int i = 0; i < thick; i++)
  {
    display.drawRoundRect(x+i, y+i, w-2*i, h-2*i, r-i, color);
    /*
    display.drawLine(x+i+r, y+i, x+i, y+i+r, color);
    display.drawLine(x+i+r, y+h-i, x+i, y+h-i-r, color);
    display.drawLine(x+w-i-r, y+i, x+w-i, y+i+r, color);
    display.drawLine(x+w-i-r, y+h-i, x+w-i, y+h-i-r, color);
    */
  }
  fillArc(x+r, y+r, 270, 90, r, r, thick-1, color);
  fillArc(x+w-r, y+r, 0, 90, r, r, thick-1, color);
  fillArc(x+r, y+h-r, 180, 90, r, r, thick-1, color);
  fillArc(x+w-r, y+h-r, 90, 90, r, r, thick-1, color);
}

void shadedThickRoundRect(int xval, int yval, int w, int h, int r, uint16_t color, int thick, int density) 
{
  display.fillRoundRect(xval, yval, w, h, r, color);
  for (int x = 0; x < w; x++){
    for (int y = 0; y < h; y++){
      if (!(x % density == 0 & y % density == 0)){
        display.drawPixel(x+xval, y+yval, GxEPD_WHITE);
      }
    }
  }
  drawThickRoundRect(xval, yval, w, h, r, color, thick);
}

void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{
  float DEG2RAD = 0.0174532925;
  int seg = 1; // Segments are 3 degrees wide = 120 segments for 360 degrees
  int inc = 1; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {
    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    display.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    display.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

    // Copy segment end to sgement start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;

    //Serial.println(i);
  }
}
