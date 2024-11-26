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