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