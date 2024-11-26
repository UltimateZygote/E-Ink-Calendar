/*
    InkyDash

    An ESP8266 powered version of InkyCal like project
    for low power desktop dashboard

    Author: tobychui

*/

/* Pin Mappings */
//Wemos D1 mini
// BUSY -> D2, RST -> D4, DC -> D3, CS -> D8, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V

// Generic ESP8266
// BUSY -> GPIO4, RST -> GPIO2, DC -> GPIO0, CS -> GPIO15, CLK -> GPIO14, DIN -> GPIO13, GND -> GND, 3.3V -> 3.3V

/*
    Deep Sleep Mode
    
    Some ESP8266 can do deep sleep but most don't
    deal to the use of CH340. Set it to true
    if you are using the bare ESP8266 module and you
    sure your module support deep sleep.
    Otherwise, leave it as false
*/
#define USE_DEEP_SLEEP false

/* 
  Display refresh interval
  
  Too frequent interval in low power mode might results getting ban by weather API / NTP servers 
  If you are not using low power mode, edit the high pwr mode interval instead
*/
#define REFRESH_INTERVAL 900e6 //900 seconds, aka 15 minutes, only work on some ESP8266
#define REFRESH_INTERVAL_HIGH_PWR_MODE 900000 //900 s, aka 15 minutes, work on all ESP8266

#define SHOW_PREVIOUS_NEXT_MONTH_DAYS true //Show previous & next months days as well

/* Display Libraries */
#include <GxEPD.h>

//#include <GxGDEW075T8/GxGDEW075T8.h>      // 7.5" b/w
//#include <GxGDEW075T7/GxGDEW075T7.h>      // 7.5" b/w 800x480
//#include <GxGDEW075Z09/GxGDEW075Z09.h>      // 7.5" b/w/r
#include <GxGDEW075Z08/GxGDEW075Z08.h>    // 7.5" b/w/r 800x480

/* Display SPI library */
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

/* Fonts */
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

/* WiFi and Connections */
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266TrueRandom.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

/* WiFi and Connections */
WiFiManager wifiManager;
WiFiUDP UDPconn;
NTPClient ntpClient(UDPconn, "pool.ntp.org");

/* SPI Settings */
GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io, /*RST=D4*/ 2, /*BUSY=D2*/ 4); // default selection of D4(=2), D2(=4)

/* Time Const */
const String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String weeksDaysFull[7] = { "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY" };
const String months[12] = {"JANUARY",  "FEBRUARY",  "MARCH",  "APRIL",  "MAY",  "JUNE",  "JULY",  "AUGUST",  "SEPTEMBER",  "OCTOBER",  "NOVEMBER",  "DECEMBER"};

/* NTP Servers*/
//Add more if you want to refresh more frequently in low power mode
const char* ntpServers[] = {
  "0.debian.pool.ntp.org",
  "1.debian.pool.ntp.org",
  "2.debian.pool.ntp.org",
  "3.debian.pool.ntp.org",
  "pool.ntp.org",
  "0.pool.ntp.org",
  "1.pool.ntp.org",
  "2.pool.ntp.org",
  "3.pool.ntp.org",
  "0.arch.pool.ntp.org",
  "1.arch.pool.ntp.org",
  "2.arch.pool.ntp.org",
  "3.arch.pool.ntp.org",
  "time.google.com",
  "time.cloudflare.com",
  "time.windows.com",
  "time.apple.com",
  "time-a-wwv.nist.gov",
  "time-b-wwv.nist.gov",
  "time-c-wwv.nist.gov"
};
const int ntpServerCounts = 20;

/* Global Variables */
const String deviceName = "InkyDash v1.0";
float currentTemp = 25.0;
float currentHumd = 50.0;
float currentRain = 0.0;
float maxTemp = 0.0;
float minTemp = 0.0;
int batRemain = 0; //In percentage


//Latitude & Longitude of your location
/* Hong Kong */
//const String lat = "22.2783";
//const String lon = "114.1747";

/* Tainan */
const String lat = "22.9908";
const String lon = "120.2133";

/* Timezone */
const String timezone = "Asia/Singapore";

void setup() {
  //Start debug serial
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up InkyDash");
  display.init(115200);

  //Set D0 to wakeup pin
  //pinMode(D0,WAKEUP_PULLUP);
  
  //Draw starting frame
  display.setRotation(3);
  /*
  if (!USE_DEEP_SLEEP) {
    display.drawPaged(drawStartingFrame);
    delay(100);
  }
  */

  // Connect to Wi-Fi
  //Hard code version
  
  //WiFi.begin("CP-IoT-Secure", "EkAmUizxnT");
  WiFi.begin("SAM FLORES", "legoland")
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print('.');
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  
  /*
  WiFi.forceSleepWake();
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("InkyDash");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
  } else {
    Serial.println("Network error");
    display.drawPaged(drawNoNetwork);
    return;
  }
  */

  delay(1000);

  //Randomly get one of the NTP server from the list and request time
  Serial.println("Resolving NTP server");
  const char* randomNtpServer = getRandomNtpServer(ntpServers);
  Serial.println("Using NTP Server: " + String(randomNtpServer));
  NTPClient timeClient(UDPconn, randomNtpServer);
  ntpClient.begin();
  ntpClient.setTimeOffset(28800);

  //Draw home page
  if (USE_DEEP_SLEEP) {
    //Get weather information from API
    updateCurrentWeatherInfo();
    //Render the calender page
    display.drawPaged(drawHomeFrame);
    Serial.println("Display updated. Entering deep sleep mode");
    ESP.deepSleep(REFRESH_INTERVAL);
    delay(100);
    //After deep sleep mode, device will reset and not entering the loop() session
  }
}

void loop() {
  //Render to e-ink, see time.ino
  datetimeUpdateCallback();
  //Enter low power mode, draw around 20mA during delay(DURATION) with WiFi off
  Serial.println("Entering low power mode");
  WiFi.forceSleepBegin();
  delay(REFRESH_INTERVAL_HIGH_PWR_MODE);
  //If your refresh interval > max integer
  /*
  int cc = REFRESH_INTERVAL_HIGH_PWR_MODE/1000;
  for (int i = 0; i < cc; i++){
    //Prevent delay overflow
    delay(1000);
  }
  */
  WiFi.forceSleepWake();
  //Exit of low power mode, draw around 80mA with WiFi ON
  Serial.println("Low power mode ended. Reconnecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }
}

/*

   Battery.ino

   Renders the battery information
   on screen

*/

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Update the battery reading from ADC pin
void updateBatteryReading() {
  int rawValue = analogRead(A0);
  // rawValue shd be within 0 - 0.854v range
  // aka 0 - 875 (ADC Output)
  float voltage = rawValue * (1 / 1023.0);
  voltage *= 4.91; //Using a 4.7k-1.2k voltage divider

  //Convert voltage to percentage
  int percentage = mapfloat(voltage, 3.2, 4.2, 0, 100);
  percentage = constrain(percentage, 0, 100);
  batRemain = percentage;
}

//Draw the battery icon at the bottom right hand corner
void drawBatteryIcon() {
  updateBatteryReading();

  //Draw the battery icon
  uint16_t barColor = GxEPD_BLACK;
  if (batRemain <= 20) {
    barColor = GxEPD_RED;
  }
  int iconBaseX = display.width() - 50;
  int iconBaseY = display.height() - 26;
  display.drawRoundRect(iconBaseX, iconBaseY, 40, 20, 3, GxEPD_BLACK);
  display.fillRoundRect(iconBaseX + 40, iconBaseY + 5, 4, 10, 3, GxEPD_BLACK);
  //Internal bar
  int maxBarWidth = 34;
  int barWidth = int(float(batRemain) / 100.0 * maxBarWidth);
  display.fillRoundRect(iconBaseX + 3, iconBaseY + 3, barWidth, 14, 3, barColor);

  //Draw percentage
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(0.5);
  String batInfo = String(batRemain) + "%";
  display.getTextBounds(batInfo, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(iconBaseX - tbw - 6, iconBaseY + tbh + 2);
  display.setTextColor(GxEPD_BLACK);
  display.print(batInfo);
  display.setTextSize(1);
}

/*
   draw.ino

   This files contains utilities for drawing to the display

*/

//Draw the frame for the home information panel
void drawHomeFrame() {
  updateNTPTimeWithRetry();
  //Reset the screen with white
  display.fillScreen(GxEPD_WHITE);
  //Draw elements
  drawDate(10, 10); //Draw the date
  drawSunMoon(display.width() - 50, 140); //Draw the day / night icon
  drawWeatherPallete(display.width() - 110, 10); //Draw current temp / humd info
  drawDayProgressBar(200, 10, GxEPD_BLACK);
  drawCalender(250);
  drawLastUpdateTimestamp();
  drawBatteryIcon();
}

//Draw a timestamp at the corner to show the last update time
void drawLastUpdateTimestamp() {
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(0.5);
  time_t epochTime = ntpClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  String currentDate = String(ptm->tm_mday) + "/" + String(ptm->tm_mon + 1) + "/" + String(ptm->tm_year + 1900) + " " + timeZeroPad(ntpClient.getHours()) + ":" + timeZeroPad(ntpClient.getMinutes()) + ":" + timeZeroPad(ntpClient.getSeconds());
  drawHighlighedText("Update: " + currentDate, 10, display.height() - 24, 5, GxEPD_WHITE, GxEPD_BLACK);
  display.setTextSize(1);
}

//Draw a day progress bar to show time inaccurately
void drawDayProgressBar(int y, int padding, uint16_t color) {
  display.fillRect(padding, y, display.width() - 2 * padding, 3, color);

  //Calculate how many percentage today has passed
  time_t epochTime = ntpClient.getEpochTime();
  int secondsSinceMidnight = epochTime % 86400; // 86400 seconds in a day (24 hours * 60 minutes * 60 seconds)
  float percentageOfDayPassed = float(secondsSinceMidnight) / 86400.0;
  int progressBarBlockSize = 3;
  //Calculate how many no of blocks needed to be rendered
  int totalNumberOfBlocks = (display.width() - 2 * padding) / (progressBarBlockSize * 2) + 1;
  int numberOfBlocksToRender = int(float(totalNumberOfBlocks) * percentageOfDayPassed);
  display.setTextColor(GxEPD_RED);
  for (int i = 0; i < numberOfBlocksToRender; i++) {
    int barHeight = 8;
    if (i == int(totalNumberOfBlocks * 0.25) || i == int(totalNumberOfBlocks * 0.5) || i == int(totalNumberOfBlocks * 0.75)) {
      barHeight = 12;
    }
    display.fillRect(padding + (i * (progressBarBlockSize * 2)), y + 5, progressBarBlockSize, barHeight, GxEPD_BLACK);
  }
  display.setTextColor(GxEPD_BLACK);
}


//Draw the date information at x,y (upper left corner point)
void drawDate(int x, int y) {
  /* Display Settings */
  display.setFont(&FreeSans24pt7b);
  display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby; uint16_t tbw, tbh;
  time_t epochTime = ntpClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);

  //Draw the month
  String currentMonth = months[ptm->tm_mon];
  display.getTextBounds(currentMonth, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(x, y + tbh);
  display.print(currentMonth);

  //Offset the Y axis downward
  y += y + tbh + 10;

  //Draw the day
  display.setTextSize(2);
  display.getTextBounds(String(ptm->tm_mday), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(x, y + tbh);
  display.print(String(ptm->tm_mday));
  display.setTextSize(1);


  //Draw weekday
  String weekDay = weeksDaysFull[ntpClient.getDay()];
  display.setFont(&FreeSans12pt7b);
  display.getTextBounds(weekDay, 0, 0, &tbx, &tby, &tbw, &tbh);
  //Offset the Y axis again before appending
  y += y + tbh + 10;
  display.setCursor(x, y + tbh);
  display.print(weekDay);

}

//Draw the calender
void drawCalender(int y) {
  // Display Settings
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  // Dimension definations
  int horizontalPadding = 10;
  int verticalPadding = 10;
  int gridWidth = int((display.width() - 2 * horizontalPadding) / 7.0);
  int gridHeight = gridWidth;
  int16_t tbx, tby; uint16_t tbw, tbh;

  // Time Calculations
  time_t epochTime = ntpClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int dayInMonth = getNumberOfDayByMonth(ptm->tm_mon, ptm->tm_year + 1900);
  int firstDayOfWeek = getFirstDayDayOfWeek() + 1;
  int dayCounter = (1 - firstDayOfWeek);

  //Render the day of week line
  display.fillRect(horizontalPadding, y - 12, display.width() - 2 * horizontalPadding, 30, GxEPD_RED);
  display.setTextColor(GxEPD_WHITE);
  for (int dx = 0; dx < 7; dx++) {
    display.getTextBounds(weekDays[dx], 0, 0, &tbx, &tby, &tbw, &tbh);
    display.setCursor(dx * gridWidth + horizontalPadding + int(tbw / 2.0), y + verticalPadding);
    display.print(weekDays[dx]);
  }
  display.setTextColor(GxEPD_BLACK);

  //Offset the y downward by 1.5 line height
  y += int(gridHeight * 1.5);

  //Get the number of days in previous month
  int previousMonthID = ptm->tm_mon;
  if (previousMonthID == 0) {
    //loop back to last year Dec
    previousMonthID = 11;
  } else {
    previousMonthID = previousMonthID - 1;
  }
  int dayInPrevMonth = getNumberOfDayByMonth(previousMonthID, ptm->tm_year + 1900);
  int nextMonthDayCounter = 1;
  
  //Render the calender dates
  display.setFont(&FreeSans12pt7b);
  int cx, cy, tcx, tcy;
  for (int dy = 0; dy < 6; dy++) {
    for (int dx = 0; dx < 7; dx++) {
      //Get the center of the grid
      cx = dx * gridWidth + horizontalPadding + int(float(gridWidth) / 2.0);
      cy = y + dy * gridHeight + verticalPadding - int(float(gridHeight) / 2.0);

      //Calculate the height and width of the text and center it at the center of the grid
      display.getTextBounds(String(dayCounter), 0, 0, &tbx, &tby, &tbw, &tbh);
      tcx = cx - (tbw / 2.0);
      tcy = cy + (tbh / 2.0);

      //Check for special case
      if (dayCounter > dayInMonth) {
        //End of this month
        if (SHOW_PREVIOUS_NEXT_MONTH_DAYS) {
          //Show next months days in another colors
          display.setTextColor(GxEPD_RED);
          display.setCursor(tcx, tcy);
          display.print(String(nextMonthDayCounter));
          display.setTextColor(GxEPD_BLACK);
          nextMonthDayCounter++;
          continue;
        } else {
          break;
        }
      }
      if (dayCounter <= 0) {
        //Shifting in first day of month
        if (SHOW_PREVIOUS_NEXT_MONTH_DAYS) {
          //Show previous months days in another colors
          display.setTextColor(GxEPD_RED);
          display.setCursor(tcx, tcy);
          display.print(String(dayInPrevMonth + dayCounter));
          display.setTextColor(GxEPD_BLACK);
        }

        dayCounter++;
        continue;
      }

      //Draw the date to screen
      if (dayCounter == ptm->tm_mday) {
        //Today
        //display.fillRoundRect(dx * gridWidth + horizontalPadding, y + dy * gridHeight + verticalPadding - gridHeight, gridWidth, gridHeight, 3, GxEPD_RED);
        display.fillCircle(cx, cy, gridWidth / 2, GxEPD_RED);
        display.setCursor(tcx, tcy);
        display.setTextColor(GxEPD_WHITE);
        display.print(String(dayCounter));
        display.setTextColor(GxEPD_BLACK);
      } else {
        display.setCursor(tcx, tcy);
        display.print(String(dayCounter));
      }

      dayCounter++;
    }
  }
}

//Draw the loading frame for the WiFi connection and settings
void drawStartingFrame() {
  //Print initializing text
  display.fillScreen(GxEPD_WHITE);
  int y_offset = -20; //Move the icon up by 20px

  //Draw imus font logo
  display.setFont(&FreeSans12pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds("imuslab", 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby + y_offset;
  display.setCursor(x, y);
  display.print("imuslab");


  //Draw initializing text
  display.setFont(&FreeSans9pt7b);
  display.getTextBounds(deviceName, 0, 0, &tbx, &tby, &tbw, &tbh);
  x = 20;
  y = display.height() - 30;
  drawHighlighedText(deviceName, x, y, 5, GxEPD_WHITE, GxEPD_BLACK);
}


//Draw a text in highlighed rectangle
void drawHighlighedText(String text, int x, int y, int padding, uint16_t text_color, uint16_t bg_color ) {
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.fillRect(uint16_t(x - padding - 2), uint16_t(y - padding - 2), uint16_t(tbw + 2 * padding + 4), uint16_t(tbh + 2 * padding), bg_color);
  display.setTextColor(text_color);
  display.setCursor(x, y + 2 * padding);
  display.print(text);
}

/*
   time.ino

   This script handle date time related features
*/


//Entry point for default mode
void datetimeUpdateCallback() {
  //Get weather information from API
  updateCurrentWeatherInfo();
  //Render the calender page
  display.drawPaged(drawHomeFrame);
}

//Update the current time with retry
void updateNTPTimeWithRetry() {
  bool succ = false;
  int counter = 0;
  while (!succ && counter < 5) {
    //Update the render information
    succ = ntpClient.update();
    if (succ) {
      //Break out of loop immediately
      break;
    }

    //NTP update failed. Retry in a few seconds
    Serial.println("Update NTP failed. Retrying in 3 seconds");
    //Stop the current NTP client
    ntpClient.end();
    delay(3000);
    //Switch another NTP server
    const char* randomNtpServer = getRandomNtpServer(ntpServers);
    Serial.println("Changing NTP Server to: " + String(randomNtpServer));
    NTPClient timeClient(UDPconn, randomNtpServer);
    ntpClient.begin();
    ntpClient.setTimeOffset(28800);
    counter++;
  }
}

//Pad any time string with 0
String timeZeroPad(int input) {
  if (input < 10) {
    return "0" + String(input);
  }
  return String(input);
}

//Get the day of week of the first day of this month
int getFirstDayDayOfWeek() {
  time_t epochTime = ntpClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  return (ntpClient.getDay() - (ptm->tm_mday % 7) + 7) % 7;
}

//Get the number of days of a month
int getNumberOfDayByMonth(int monthId, int currentYear) {
  if (monthId < 0 || monthId > 11) {
    return -1; // Invalid monthId
  }
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (monthId == 1 && isLeapYear(currentYear)){
    //Feb in leap year
    return 29;
  }
  return daysInMonth[monthId];
}

//Return if the current time is night
bool isNight() {
  int currentHour = ntpClient.getHours();
  // Night is defined from 7:00 PM to 7:00 AM (19:00 to 7:00 in 24-hour format)
  // Change this to fit your life style
  return (currentHour >= 19 || currentHour < 7);
}

//Check if given year is leap year
bool isLeapYear(int year) {
    // Check if the year is divisible by 4
    if (year % 4 == 0) {
        // If it's divisible by 100, it should also be divisible by 400 to be a leap year
        if (year % 100 == 0) {
            return year % 400 == 0;
        }
        return true;
    }
    return false;
}

//Grab a random NTP server
const char* getRandomNtpServer(const char* ntpServers[]) {
  int randomIndex = ESP8266TrueRandom.random(ntpServerCounts - 1);
  //int randomIndex = 0;
  // Return the NTP server at the randomly chosen index
  return ntpServers[randomIndex];
}

/*
 * 
 * utils.ino
 * 
 * Handle utilities functions
 */


 //Instruction for WiFi setup
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("WiFi not setup or not found. Entered config mode");
  display.drawPaged(drawWifiSetupInstruction);
}

//Draw wifi connection error frame
void drawWifiSetupInstruction() {
  //Print initializing text
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.fillScreen(GxEPD_WHITE);

  //Render some style
  display.fillRect(0, 0, display.width(), 60, GxEPD_BLACK);
   
  //Render instructions
  display.setFont(&FreeSans12pt7b);
  display.setTextColor(GxEPD_WHITE);
  int x = 20;
  int y = 40;
  display.setCursor(x, y);
  display.print("WiFi Setup Instruction");
  display.setTextColor(GxEPD_BLACK);
  display.println();
  display.println();
  display.setFont(&FreeSans9pt7b);
  display.println("1. Connect to WiFi w/ SSID \"InkyDash\"");
  String apIp = WiFi.softAPIP().toString();
  display.println("2. Open http://" + apIp + " with web browser");
  display.println("3. Enter your home WiFi SSID & password");
  display.println("4. Save and wait for device to reboot");

  display.println();
  display.println();
  display.println("This page will refresh automatically after the setup is completed.");
  display.println();
  display.setTextColor(GxEPD_RED);
  display.println(deviceName);
  
  //Draw initializing text
  display.setFont(&FreeSans9pt7b);
  display.getTextBounds("WiFi Setup Mode", 0, 0, &tbx, &tby, &tbw, &tbh);
  x = 20;
  y = display.height() - 30;
  drawHighlighedText("WiFi Setup Mode", x, y, 5, GxEPD_WHITE, GxEPD_RED);
}

//Draw the loading frame for the WiFi connection and settings
void drawNoNetwork() {
  //Print initializing text
  display.fillScreen(GxEPD_WHITE);
  int y_offset = -20; //Move the icon up by 20px

  //Draw imus font logo
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds("NETWORK ERROR", 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby + y_offset;
  display.setCursor(x, y);
  display.print("NETWORK ERROR");


  //Draw initializing text
  display.setFont(&FreeSans9pt7b);
  display.getTextBounds(deviceName, 0, 0, &tbx, &tby, &tbw, &tbh);
  x = 20;
  y = display.height() - 30;
  drawHighlighedText(deviceName, x, y, 5, GxEPD_WHITE, GxEPD_BLACK);
}

/*
    Weather Access API
    Information provided by the open-meteo.com open data API

*/

//Draw weather pallete, by default it align right
void drawWeatherPallete(int x, int y) {
  display.setFont(&FreeSans18pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;

  //Print current temperature and humd
  display.getTextBounds(String(currentTemp, 1) + " C", 0, 0, &tbx, &tby, &tbw, &tbh);
  int tempX = display.width() - tbw - 10;
  display.setCursor(tempX, y + tbh);
  display.print(String(currentTemp, 1) + " C");
  //Draw the degree sign as it is not supported in ASCII
  display.getTextBounds(String(currentTemp, 1), 0, 0, &tbx, &tby, &tbw, &tbh);
  display.fillCircle(tempX + tbw + 8, y + 5, 5, GxEPD_BLACK);
  display.fillCircle(tempX + tbw + 8, y + 5, 2, GxEPD_WHITE);
  
  //Min max daily temp
  y += tbh + 10;
  display.setFont(&FreeSans9pt7b);
  String detailInfo = String(minTemp, 1) + " / " + String(maxTemp, 1) + "'C";
  display.getTextBounds(detailInfo, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(display.width() - tbw - 10, y + tbh);
  display.print(detailInfo);

  //Humidity
  y += tbh + 10;
  detailInfo = "RH " + String(currentHumd,0) + "%";
  display.getTextBounds(detailInfo, 0, 0, &tbx, &tby, &tbw, &tbh);
  display.setCursor(display.width() - tbw - 10, y + tbh);
  display.print(detailInfo);

}



//Draw the sun or moon icon base on day / night time
void drawSunMoon(int x, int y) {
  if (isNight()) {
    //Draw moon
    display.fillCircle(x, y, 40, GxEPD_RED);
    display.fillCircle(x - 30, y - 15, 38, GxEPD_WHITE);
  } else {
    //Draw sun
    display.fillCircle(x, y, 40, GxEPD_RED);
    //some clouds
    display.fillCircle(x + 10, y + 28, 20, GxEPD_WHITE);
    display.fillCircle(x + 40, y + 20, 30, GxEPD_WHITE);
  }
}

bool updateCurrentWeatherInfo() {
  // Make HTTP request
  HTTPClient http;
  WiFiClient client;
  http.begin(client, "http://api.open-meteo.com/v1/forecast?latitude=" + lat + "&longitude=" + lon + "&current=temperature_2m,relative_humidity_2m,rain&daily=temperature_2m_max,temperature_2m_min&timezone=" + timezone + "&forecast_days=1");
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      // Parse JSON
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      // Extract temperature_2m and relative_humidity_2m
      if (!error) {
        currentTemp = doc["current"]["temperature_2m"];
        currentHumd = doc["current"]["relative_humidity_2m"];
        currentRain = doc["current"]["rain"];
        minTemp = doc["daily"]["temperature_2m_min"][0];
        maxTemp = doc["daily"]["temperature_2m_max"][0];
        

        Serial.print("Temperature: ");
        Serial.print(currentTemp);
        Serial.println("°C");

        Serial.print("Humidity: ");
        Serial.print(currentHumd);
        Serial.println("%");

        Serial.print("Rain: ");
        Serial.print(currentRain);
        Serial.println("mm");
      } else {
        Serial.println("Failed to parse JSON");
        return false;
      }
    } else {
      Serial.printf("HTTP error code: %d\n", httpCode);
      return false;
    }
  } else {
    Serial.println("Failed to connect to server");
    return false;
  }

  http.end();
  return true;
}