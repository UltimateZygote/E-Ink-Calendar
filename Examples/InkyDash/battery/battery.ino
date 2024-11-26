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