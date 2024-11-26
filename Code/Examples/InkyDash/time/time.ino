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