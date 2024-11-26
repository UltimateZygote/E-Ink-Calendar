/*
    Weather Access API
    Information provided by the open-meteo.com open data API

*/

//Latitude & Longitude of your location
/* Hong Kong */
//const String lat = "22.2783";
//const String lon = "114.1747";

/* Tainan */
const String lat = "22.9908";
const String lon = "120.2133";

/* Timezone */
const String timezone = "Asia/Singapore";

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