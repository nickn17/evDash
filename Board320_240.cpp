#ifndef BOARD320_240_CPP
#define BOARD320_240_CPP

#include <analogWrite.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "BoardInterface.h"
#include "Board320_240.h"

/**
   Init board
*/
void Board320_240::initBoard() {

  // Set button pins for input
  pinMode(this->pinButtonMiddle, INPUT);
  pinMode(this->pinButtonLeft, INPUT);
  pinMode(this->pinButtonRight, INPUT);

  // mute speaker
  if (this->pinSpeaker != 0) {
    Serial.println("Mute speaker for m5stack");
    dacWrite(this->pinSpeaker, 0);
  }

  // Init display
  Serial.println("Init this->tft display");
  this->tft.begin();
  this->tft.invertDisplay(this->invertDisplay);
  this->tft.setRotation(this->liveData->settings.displayRotation);
  this->setBrightness((this->liveData->settings.lcdBrightness == 0) ? 100 : this->liveData->settings.lcdBrightness);
  this->tft.fillScreen(TFT_BLACK);

  bool psramUsed = false; // 320x240 16bpp sprites requires psram
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
  if (psramFound())
    psramUsed = true;
#endif
  this->spr.setColorDepth((psramUsed) ? 16 : 8);
  this->spr.createSprite(320, 240);
}

/**
   After setup device
*/
void Board320_240::afterSetup() {
  // Show test data on right button during boot device
  this->displayScreen = this->liveData->settings.defaultScreen;
  if (digitalRead(this->pinButtonRight) == LOW) {
    this->loadTestData();
  }
}

/**
   Set brightness level
*/
void Board320_240::setBrightness(byte lcdBrightnessPerc) {

  analogWrite(TFT_BL, lcdBrightnessPerc);
}

/**
   Clear screen a display two lines message
*/
void Board320_240::displayMessage(const char* row1, const char* row2) {

  // Must draw directly, withou sprite (due to psramFound check)
  this->tft.fillScreen(TFT_BLACK);
  this->tft.setTextDatum(ML_DATUM);
  this->tft.setTextColor(TFT_WHITE, TFT_BLACK);
  this->tft.setFreeFont(&Roboto_Thin_24);
  this->tft.setTextDatum(BL_DATUM);
  this->tft.drawString(row1, 0, 240 / 2, GFXFF);
  this->tft.drawString(row2, 0, (240 / 2) + 30, GFXFF);
}

/**
  Draw cell on dashboard
*/
void Board320_240::drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, uint16_t bgColor, uint16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  this->spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  bgColor);
  this->spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  this->spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
  this->spr.setTextDatum(TL_DATUM); // Topleft
  this->spr.setTextColor(TFT_SILVER, bgColor); // Bk, fg color
  this->spr.setTextSize(1); // Size for small 5x7 font
  this->spr.drawString(desc, posx, posy, 2);

  // Big 2x2 cell in the middle of screen
  if (w == 2 && h == 2) {

    // Bottom 2 numbers with charged/discharged kWh from start
    posx = (x * 80) + 5;
    posy = ((y + h) * 60) - 32;
    sprintf(this->tmpStr3, "-%01.01f", this->liveData->params.cumulativeEnergyDischargedKWh - this->liveData->params.cumulativeEnergyDischargedKWhStart);
    this->spr.setFreeFont(&Roboto_Thin_24);
    this->spr.setTextDatum(TL_DATUM);
    this->spr.drawString(this->tmpStr3, posx, posy, GFXFF);

    posx = ((x + w) * 80) - 8;
    sprintf(this->tmpStr3, "+%01.01f", this->liveData->params.cumulativeEnergyChargedKWh - this->liveData->params.cumulativeEnergyChargedKWhStart);
    this->spr.setTextDatum(TR_DATUM);
    this->spr.drawString(this->tmpStr3, posx, posy, GFXFF);

    // Main number - kwh on roads, amps on charges
    posy = (y * 60) + 24;
    this->spr.setTextColor(fgColor, bgColor);
    this->spr.setFreeFont(&Orbitron_Light_32);
    this->spr.drawString(text, posx, posy, 7);

  } else {

    // All others 1x1 cells
    this->spr.setTextDatum(MC_DATUM);
    this->spr.setTextColor(fgColor, bgColor);
    this->spr.setFreeFont(&Orbitron_Light_24);
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    this->spr.drawString(text, posx, posy, (w == 2 ? 7 : GFXFF));
  }
}

/**
  Draw small rect 80x30
*/
void Board320_240::drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, int16_t bgColor, int16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 32) + 1;

  this->spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
  this->spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 32) - 1, h * 32, TFT_BLACK);
  this->spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 32) - 1, w * 80, TFT_BLACK);
  this->spr.setTextDatum(TL_DATUM); // Topleft
  this->spr.setTextColor(TFT_SILVER, bgColor); // Bk, fg bgColor
  this->spr.setTextSize(1); // Size for small 5x7 font
  this->spr.drawString(desc, posx, posy, 2);

  this->spr.setTextDatum(TC_DATUM);
  this->spr.setTextColor(fgColor, bgColor);
  posx = (x * 80) + (w * 80 / 2) - 3;
  this->spr.drawString(text, posx, posy + 14, 2);
}

/**
  Show tire pressures / temperatures
  Custom field
*/
void Board320_240::showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char* topleft, const char* topright, const char* bottomleft, const char* bottomright, uint16_t color) {

  int32_t posx, posy;

  this->spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  color);
  this->spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  this->spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);

  this->spr.setTextDatum(TL_DATUM);
  this->spr.setTextColor(TFT_SILVER, color);
  this->spr.setTextSize(1);
  posx = (x * 80) + 4;
  posy = (y * 60) + 0;
  this->spr.drawString(topleft, posx, posy, 2);
  posy = (y * 60) + 14;
  this->spr.drawString(bottomleft, posx, posy, 2);

  this->spr.setTextDatum(TR_DATUM);
  posx = ((x + w) * 80) - 4;
  posy = (y * 60) + 0;
  this->spr.drawString(topright, posx, posy, 2);
  posy = (y * 60) + 14;
  this->spr.drawString(bottomright, posx, posy, 2);
}

/**
   Main screen (Screen 1)
*/
void Board320_240::drawSceneMain() {

  // Tire pressure
  char pressureStr[4] = "bar";
  char temperatureStr[2] = "C";
  if (this->liveData->settings.pressureUnit != 'b')
    strcpy(pressureStr, "psi");
  if (this->liveData->settings.temperatureUnit != 'c')
    strcpy(temperatureStr, "F");
  sprintf(this->tmpStr1, "%01.01f%s %02.00f%s", this->liveData->bar2pressure(this->liveData->params.tireFrontLeftPressureBar), pressureStr, this->liveData->celsius2temperature(this->liveData->params.tireFrontLeftTempC), temperatureStr);
  sprintf(this->tmpStr2, "%02.00f%s %01.01f%s", this->liveData->celsius2temperature(this->liveData->params.tireFrontRightTempC), temperatureStr, this->liveData->bar2pressure(this->liveData->params.tireFrontRightPressureBar), pressureStr);
  sprintf(this->tmpStr3, "%01.01f%s %02.00f%s", this->liveData->bar2pressure(this->liveData->params.tireRearLeftPressureBar), pressureStr, this->liveData->celsius2temperature(this->liveData->params.tireRearLeftTempC), temperatureStr);
  sprintf(this->tmpStr4, "%02.00f%s %01.01f%s", this->liveData->celsius2temperature(this->liveData->params.tireRearRightTempC), temperatureStr, this->liveData->bar2pressure(this->liveData->params.tireRearRightPressureBar), pressureStr);
  showTires(1, 0, 2, 1, this->tmpStr1, this->tmpStr2, this->tmpStr3, this->tmpStr4, TFT_BLACK);

  // Added later - kwh total in tires box
  // TODO: refactoring
  this->spr.setTextDatum(TL_DATUM);
  this->spr.setTextColor(TFT_GREEN, TFT_BLACK);
  sprintf(this->tmpStr1, "C: %01.01f +%01.01fkWh", this->liveData->params.cumulativeEnergyChargedKWh, this->liveData->params.cumulativeEnergyChargedKWh - this->liveData->params.cumulativeEnergyChargedKWhStart);
  this->spr.drawString(this->tmpStr1, (1 * 80) + 4, (0 * 60) + 30, 2);
  this->spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  sprintf(this->tmpStr1, "D: %01.01f -%01.01fkWh", this->liveData->params.cumulativeEnergyDischargedKWh, this->liveData->params.cumulativeEnergyDischargedKWh - this->liveData->params.cumulativeEnergyDischargedKWhStart);
  this->spr.drawString(this->tmpStr1, (1 * 80) + 4, (0 * 60) + 44, 2);

  // batPowerKwh100 on roads, else batPowerAmp
  if (this->liveData->params.speedKmh > 20) {
    sprintf(this->tmpStr1, "%01.01f", this->liveData->km2distance(this->liveData->params.batPowerKwh100));
    drawBigCell(1, 1, 2, 2, this->tmpStr1, ((this->liveData->settings.distanceUnit == 'k') ? "POWER KWH/100KM" : "POWER KWH/100MI"), (this->liveData->params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (this->liveData->params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  } else {
    // batPowerAmp on chargers (under 10kmh)
    sprintf(this->tmpStr1, "%01.01f", this->liveData->params.batPowerKw);
    drawBigCell(1, 1, 2, 2, this->tmpStr1, "POWER KW", (this->liveData->params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (this->liveData->params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }

  // socPerc
  sprintf(this->tmpStr1, "%01.00f%%", this->liveData->params.socPerc);
  sprintf(this->tmpStr2, (this->liveData->params.sohPerc ==  100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), this->liveData->params.sohPerc);
  drawBigCell(0, 0, 1, 1, this->tmpStr1, this->tmpStr2, (this->liveData->params.socPerc < 10 || this->liveData->params.sohPerc < 100 ? TFT_RED : (this->liveData->params.socPerc  > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)), TFT_WHITE);

  // batPowerAmp
  sprintf(this->tmpStr1, (abs(this->liveData->params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), this->liveData->params.batPowerAmp);
  drawBigCell(0, 1, 1, 1, this->tmpStr1, "CURRENT A", (this->liveData->params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // batVoltage
  sprintf(this->tmpStr1, "%03.00f", this->liveData->params.batVoltage);
  drawBigCell(0, 2, 1, 1, this->tmpStr1, "VOLTAGE", TFT_DEFAULT_BK, TFT_WHITE);

  // batCellMinV
  sprintf(this->tmpStr1, "%01.02f", this->liveData->params.batCellMaxV - this->liveData->params.batCellMinV);
  sprintf(this->tmpStr2, "CELLS %01.02f", this->liveData->params.batCellMinV);
  drawBigCell(0, 3, 1, 1, ( this->liveData->params.batCellMaxV - this->liveData->params.batCellMinV == 0.00 ? "OK" : this->tmpStr1), this->tmpStr2, TFT_DEFAULT_BK, TFT_WHITE);

  // batTempC
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), this->liveData->celsius2temperature(this->liveData->params.batMinC));
  sprintf(this->tmpStr2, ((this->liveData->settings.temperatureUnit == 'c') ? "BATT. %01.00fC" : "BATT. %01.01fF"), this->liveData->celsius2temperature(this->liveData->params.batMaxC));
  drawBigCell(1, 3, 1, 1, this->tmpStr1, this->tmpStr2, TFT_TEMP, (this->liveData->params.batTempC >= 15) ? ((this->liveData->params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);

  // batHeaterC
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), this->liveData->celsius2temperature(this->liveData->params.batHeaterC));
  drawBigCell(2, 3, 1, 1, this->tmpStr1, "BAT.HEAT", TFT_TEMP, TFT_WHITE);

  // Aux perc
  sprintf(this->tmpStr1, "%01.00f%%", this->liveData->params.auxPerc);
  drawBigCell(3, 0, 1, 1, this->tmpStr1, "AUX BAT.", (this->liveData->params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);

  // Aux amp
  sprintf(this->tmpStr1, (abs(this->liveData->params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), this->liveData->params.auxCurrentAmp);
  drawBigCell(3, 1, 1, 1, this->tmpStr1, "AUX AMPS",  (this->liveData->params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // auxVoltage
  sprintf(this->tmpStr1, "%01.01f", this->liveData->params.auxVoltage);
  drawBigCell(3, 2, 1, 1, this->tmpStr1, "AUX VOLTS", (this->liveData->params.auxVoltage < 12.1 ? TFT_RED : (this->liveData->params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)), TFT_WHITE);

  // indoorTemperature
  sprintf(this->tmpStr1, "%01.01f", this->liveData->celsius2temperature(this->liveData->params.indoorTemperature));
  sprintf(this->tmpStr2, "IN/OUT%01.01f", this->liveData->celsius2temperature(this->liveData->params.outdoorTemperature));
  drawBigCell(3, 3, 1, 1, this->tmpStr1, this->tmpStr2, TFT_TEMP, TFT_WHITE);
}

/**
   Speed + kwh/100km (Screen 2)
*/
void Board320_240::drawSceneSpeed() {

  int32_t posx, posy;

  // HUD
  if (this->displayScreenSpeedHud) {

    // Change rotation to vertical & mirror
    if (this->tft.getRotation() != 6) {
      this->tft.setRotation(6);
    }

    this->tft.fillScreen(TFT_BLACK);
    this->tft.setTextDatum(TR_DATUM); // top-right alignment
    this->tft.setTextColor(TFT_WHITE, TFT_BLACK); // foreground, background text color

    // Draw speed
    this->tft.setTextSize((this->liveData->params.speedKmh > 99) ? 1 : 2);
    sprintf(this->tmpStr3, "0");
    if (this->liveData->params.speedKmh > 10)
      sprintf(this->tmpStr3, "%01.00f", this->liveData->km2distance(this->liveData->params.speedKmh));
    this->tft.drawString(this->tmpStr3, 240, 0, 8);

    // Draw power kWh/100km (>25kmh) else kW
    this->tft.setTextSize(1);
    if (this->liveData->params.speedKmh > 25 && this->liveData->params.batPowerKw < 0)
      sprintf(this->tmpStr3, "%01.01f", this->liveData->km2distance(this->liveData->params.batPowerKwh100));
    else
      sprintf(this->tmpStr3, "%01.01f", this->liveData->params.batPowerKw);
    this->tft.drawString(this->tmpStr3, 240, 150, 8);

    // Draw soc%
    sprintf(this->tmpStr3, "%01.00f", this->liveData->params.socPerc);
    this->tft.drawString(this->tmpStr3, 240 , 230, 8);

    // Cold gate cirlce
    this->tft.fillCircle(30, 280, 25, (this->liveData->params.batTempC >= 15) ? ((this->liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);

    // Brake lights
    this->tft.fillRect(0, 310, 240, 10, (this->liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);

    return;
  }

  //
  this->spr.fillRect(0, 36, 200, 160, TFT_DARKRED);

  posx = 320 / 2;
  posy = 40;
  this->spr.setTextDatum(TR_DATUM);
  this->spr.setTextColor(TFT_WHITE, TFT_DARKRED);
  this->spr.setTextSize(2); // Size for small 5cix7 font
  sprintf(this->tmpStr3, "0");
  if (this->liveData->params.speedKmh > 10)
    sprintf(this->tmpStr3, "%01.00f", this->liveData->km2distance(this->liveData->params.speedKmh));
  this->spr.drawString(this->tmpStr3, 200, posy, 7);

  posy = 145;
  this->spr.setTextDatum(TR_DATUM); // Top center
  this->spr.setTextSize(1);
  if (this->liveData->params.speedKmh > 25 && this->liveData->params.batPowerKw < 0) {
    sprintf(this->tmpStr3, "%01.01f", this->liveData->km2distance(this->liveData->params.batPowerKwh100));
  } else {
    sprintf(this->tmpStr3, "%01.01f", this->liveData->params.batPowerKw);
  }
  this->spr.drawString(this->tmpStr3, 200, posy, 7);

  // Bottom 2 numbers with charged/discharged kWh from start
  this->spr.setFreeFont(&Roboto_Thin_24);
  this->spr.setTextColor(TFT_WHITE, TFT_BLACK);
  posx = 5;
  posy = 5;
  this->spr.setTextDatum(TL_DATUM);
  sprintf(this->tmpStr3, ((this->liveData->settings.distanceUnit == 'k') ? "%01.00fkm  " : "%01.00fmi  "), this->liveData->km2distance(this->liveData->params.odoKm));
  this->spr.drawString(this->tmpStr3, posx, posy, GFXFF);
  if (this->liveData->params.motorRpm > -1) {
    this->spr.setTextDatum(TR_DATUM);
    sprintf(this->tmpStr3, "     %01.00frpm" , this->liveData->params.motorRpm);
    this->spr.drawString(this->tmpStr3, 320 - posx, posy, GFXFF);
  }

  // Bottom info
  // Cummulative regen/power
  posy = 240 - 5;
  sprintf(this->tmpStr3, "-%01.01f    ", this->liveData->params.cumulativeEnergyDischargedKWh - this->liveData->params.cumulativeEnergyDischargedKWhStart);
  this->spr.setTextDatum(BL_DATUM);
  this->spr.drawString(this->tmpStr3, posx, posy, GFXFF);
  posx = 320 - 5;
  sprintf(this->tmpStr3, "    +%01.01f", this->liveData->params.cumulativeEnergyChargedKWh - this->liveData->params.cumulativeEnergyChargedKWhStart);
  this->spr.setTextDatum(BR_DATUM);
  this->spr.drawString(this->tmpStr3, posx, posy, GFXFF);
  // Bat.power
  posx = 320 / 2;
  sprintf(this->tmpStr3, "   %01.01fkw   ", this->liveData->params.batPowerKw);
  this->spr.setTextDatum(BC_DATUM);
  this->spr.drawString(this->tmpStr3, posx, posy, GFXFF);

  // RIGHT INFO
  // Battery "cold gate" detection - red < 15C (43KW limit), <25 (blue - 55kW limit), green all ok
  this->spr.fillCircle(290, 60, 25, (this->liveData->params.batTempC >= 15) ? ((this->liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  this->spr.setTextColor(TFT_WHITE, (this->liveData->params.batTempC >= 15) ? ((this->liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  this->spr.setFreeFont(&Roboto_Thin_24);
  this->spr.setTextDatum(MC_DATUM);
  sprintf(this->tmpStr3, "%01.00f", this->liveData->celsius2temperature(this->liveData->params.batTempC));
  this->spr.drawString(this->tmpStr3, 290, 60, GFXFF);
  // Brake lights
  this->spr.fillRect(210, 40, 40, 40, (this->liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);

  // Soc%, bat.kWh
  this->spr.setFreeFont(&Orbitron_Light_32);
  this->spr.setTextColor(TFT_WHITE, TFT_BLACK);
  this->spr.setTextDatum(TR_DATUM);
  sprintf(this->tmpStr3, " %01.00f%%", this->liveData->params.socPerc);
  this->spr.drawString(this->tmpStr3, 320, 94, GFXFF);
  if (this->liveData->params.socPerc > 0) {
    float capacity = this->liveData->params.batteryTotalAvailableKWh * (this->liveData->params.socPerc / 100);
    // calibration for Niro/Kona, real available capacity is ~66.5kWh, 0-10% ~6.2kWh, 90-100% ~7.2kWh
    if (this->liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || this->liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64) {
      capacity = (this->liveData->params.socPerc * 0.615) * (1 + (this->liveData->params.socPerc * 0.0008));
    }
    sprintf(this->tmpStr3, " %01.01f", capacity);
    this->spr.drawString(this->tmpStr3, 320, 129, GFXFF);
    this->spr.drawString("kWh", 320, 164, GFXFF);
  }
}

/**
   Battery cells (Screen 3)
*/
void Board320_240::drawSceneBatteryCells() {

  int32_t posx, posy;

  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batHeaterC));
  drawSmallCell(0, 0, 1, 1, this->tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batInletC));
  drawSmallCell(1, 0, 1, 1, this->tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[0]));
  drawSmallCell(0, 1, 1, 1, this->tmpStr1, "MO1", TFT_TEMP, (this->liveData->params.batModuleTempC[0] >= 15) ? ((this->liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[1]));
  drawSmallCell(1, 1, 1, 1, this->tmpStr1, "MO2", TFT_TEMP, (this->liveData->params.batModuleTempC[1] >= 15) ? ((this->liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[2]));
  drawSmallCell(2, 1, 1, 1, this->tmpStr1, "MO3", TFT_TEMP, (this->liveData->params.batModuleTempC[2] >= 15) ? ((this->liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[3]));
  drawSmallCell(3, 1, 1, 1, this->tmpStr1, "MO4", TFT_TEMP, (this->liveData->params.batModuleTempC[3] >= 15) ? ((this->liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  // Ioniq (up to 12 cells)
  for (uint16_t i = 4; i < this->liveData->params.batModuleTempCount; i++) {
    if (this->liveData->params.batModuleTempC[i] == 0)
      continue;
    posx = (((i - 4) % 8) * 40);
    posy = ((floor((i - 4) / 8)) * 13) + 64;
    //this->spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
    this->spr.setTextSize(1); // Size for small 5x7 font
    this->spr.setTextDatum(TL_DATUM);
    this->spr.setTextColor(((this->liveData->params.batModuleTempC[i] >= 15) ? ((this->liveData->params.batModuleTempC[i] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED), TFT_BLACK);
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00fC" : "%01.01fF"), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[i]));
    this->spr.drawString(this->tmpStr1, posx + 4, posy, 2);
  }

  this->spr.setTextDatum(TL_DATUM); // Topleft
  this->spr.setTextSize(1); // Size for small 5x7 font

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < 98; i++) {
    if ((this->liveData->params.cellVoltage[i] < minVal || minVal == -1) && this->liveData->params.cellVoltage[i] != -1)
      minVal = this->liveData->params.cellVoltage[i];
    if ((this->liveData->params.cellVoltage[i] > maxVal || maxVal == -1) && this->liveData->params.cellVoltage[i] != -1)
      maxVal = this->liveData->params.cellVoltage[i];
    if (this->liveData->params.cellVoltage[i] > 0 && i > this->liveData->params.cellCount + 1)
      this->liveData->params.cellCount = i + 1;
  }

  // Draw cell matrix
  for (int i = 0; i < 98; i++) {
    if (this->liveData->params.cellVoltage[i] == -1)
      continue;
    posx = ((i % 8) * 40) + 4;
    posy = ((floor(i / 8) + (this->liveData->params.cellCount > 96 ? 0 : 1)) * 13) + 68;
    sprintf(this->tmpStr3, "%01.02f", this->liveData->params.cellVoltage[i]);
    this->spr.setTextColor(TFT_NAVY, TFT_BLACK);
    if (this->liveData->params.cellVoltage[i] == minVal && minVal != maxVal)
      this->spr.setTextColor(TFT_RED, TFT_BLACK);
    if (this->liveData->params.cellVoltage[i] == maxVal && minVal != maxVal)
      this->spr.setTextColor(TFT_GREEN, TFT_BLACK);
    this->spr.drawString(this->tmpStr3, posx, posy, 2);
  }
}

/**
   drawPreDrawnChargingGraphs
   P = U.I
*/
void Board320_240::drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, int mulY) {

  // Rapid gate
  this->spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_RAPIDGATE35);
  // Coldgate <5C
  this->spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE0_5);
  // Coldgate 5-14C
  this->spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 40 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  // Coldgate 15-24C
  this->spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  this->spr.drawLine(zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  // Optimal
  this->spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 51 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 51 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 53 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 53 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 55 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 55 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 71 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 71 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 73 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 73 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 75 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 75 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX + (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX + (/* SOC FROM */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  this->spr.drawLine(zeroX +  (/* SOC FROM */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY),
                     zeroX +  (/* SOC TO */ 100 * mulX), zeroY - (/*I*/ 15 * /*U SOC*/ (100 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  // Triangles
  int x = zeroX;
  int y;
  if (this->liveData->params.batMaxC >= 35) {
    y = zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (this->liveData->params.batMinC >= 25) {
    y = zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (this->liveData->params.batMinC >= 15) {
    y = zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (this->liveData->params.batMinC >= 5) {
    y = zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else {
    y = zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  this->spr.fillTriangle(x + 5, y,  x , y - 5, x, y + 5, TFT_ORANGE);
}

/**
   Charging graph (Screen 4)
*/
void Board320_240::drawSceneChargingGraph() {

  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3; // 100% = 300px;
  int mulY = 2; // 100kW = 200px
  int maxKw = 80;
  int posy = 0;
  uint16_t color;

  this->spr.fillSprite(TFT_BLACK);

  sprintf(this->tmpStr1, "%01.00f", this->liveData->params.socPerc);
  drawSmallCell(0, 0, 1, 1, this->tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);
  sprintf(this->tmpStr1, "%01.01f", this->liveData->params.batPowerKw);
  drawSmallCell(1, 0, 1, 1, this->tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);
  sprintf(this->tmpStr1, "%01.01f", this->liveData->params.batPowerAmp);
  drawSmallCell(2, 0, 1, 1, this->tmpStr1, "CURRENT A", TFT_TEMP, TFT_CYAN);
  sprintf(this->tmpStr1, "%03.00f", this->liveData->params.batVoltage);
  drawSmallCell(3, 0, 1, 1, this->tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batHeaterC));
  drawSmallCell(0, 1, 1, 1, this->tmpStr1, "HEATER", TFT_TEMP, TFT_RED);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batInletC));
  drawSmallCell(1, 1, 1, 1, this->tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.batMinC));
  drawSmallCell(2, 1, 1, 1, this->tmpStr1, "BAT.MIN", (this->liveData->params.batMinC >= 15) ? ((this->liveData->params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), this->liveData->celsius2temperature(this->liveData->params.outdoorTemperature));
  drawSmallCell(3, 1, 1, 1, this->tmpStr1, "OUT.TEMP.", TFT_TEMP, TFT_CYAN);

  this->spr.setTextColor(TFT_SILVER, TFT_TEMP);

  for (int i = 0; i <= 10; i++) {
    color = TFT_DARKRED2;
    if (i == 0 || i == 5 || i == 10)
      color = TFT_DARKRED;
    this->spr.drawFastVLine(zeroX + (i * 10 * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
    /*if (i != 0 && i != 10) {
      sprintf(this->tmpStr1, "%d%%", i * 10);
      this->spr.setTextDatum(BC_DATUM);
      this->spr.drawString(this->tmpStr1, zeroX + (i * 10 * mulX),  zeroY - (maxKw * mulY), 2);
      }*/
    if (i <= (maxKw / 10)) {
      this->spr.drawFastHLine(zeroX, zeroY - (i * 10 * mulY), 100 * mulX, color);
      if (i > 0) {
        sprintf(this->tmpStr1, "%d", i * 10);
        this->spr.setTextDatum(ML_DATUM);
        this->spr.drawString(this->tmpStr1, zeroX + (100 * mulX) + 3, zeroY - (i * 10 * mulY), 2);
      }
    }
  }

  // Draw suggested curves
  if (this->liveData->settings.predrawnChargingGraphs == 1) {
    drawPreDrawnChargingGraphs(zeroX, zeroY, mulX, mulY);
  }

  // Draw realtime values
  for (int i = 0; i <= 100; i++) {
    if (this->liveData->params.chargingGraphBatMinTempC[i] > -10)
      this->spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (this->liveData->params.chargingGraphBatMinTempC[i]*mulY), mulX, TFT_BLUE);
    if (this->liveData->params.chargingGraphBatMaxTempC[i] > -10)
      this->spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (this->liveData->params.chargingGraphBatMaxTempC[i]*mulY), mulX, TFT_BLUE);
    if (this->liveData->params.chargingGraphWaterCoolantTempC[i] > -10)
      this->spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (this->liveData->params.chargingGraphWaterCoolantTempC[i]*mulY), mulX, TFT_PURPLE);
    if (this->liveData->params.chargingGraphHeaterTempC[i] > -10)
      this->spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (this->liveData->params.chargingGraphHeaterTempC[i]*mulY), mulX, TFT_RED);

    if (this->liveData->params.chargingGraphMinKw[i] > 0)
      this->spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (this->liveData->params.chargingGraphMinKw[i]*mulY), mulX, TFT_GREENYELLOW);
    if (this->liveData->params.chargingGraphMaxKw[i] > 0)
      this->spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (this->liveData->params.chargingGraphMaxKw[i]*mulY), mulX, TFT_YELLOW);
  }

  // Bat.module temperatures
  this->spr.setTextSize(1); // Size for small 5x7 font
  this->spr.setTextDatum(BL_DATUM);
  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "1=%01.00fC " : "1=%01.00fF "), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[0]));
  this->spr.setTextColor((this->liveData->params.batModuleTempC[0] >= 15) ? ((this->liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  this->spr.drawString(this->tmpStr1, 0,  zeroY - (maxKw * mulY), 2);

  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "2=%01.00fC " : "2=%01.00fF "), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[1]));
  this->spr.setTextColor((this->liveData->params.batModuleTempC[1] >= 15) ? ((this->liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  this->spr.drawString(this->tmpStr1, 48,  zeroY - (maxKw * mulY), 2);

  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "3=%01.00fC " : "3=%01.00fF "), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[2]));
  this->spr.setTextColor((this->liveData->params.batModuleTempC[2] >= 15) ? ((this->liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  this->spr.drawString(this->tmpStr1, 96,  zeroY - (maxKw * mulY), 2);

  sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "4=%01.00fC " : "4=%01.00fF "), this->liveData->celsius2temperature(this->liveData->params.batModuleTempC[3]));
  this->spr.setTextColor((this->liveData->params.batModuleTempC[3] >= 15) ? ((this->liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  this->spr.drawString(this->tmpStr1, 144,  zeroY - (maxKw * mulY), 2);
  sprintf(this->tmpStr1, "ir %01.00fkOhm", this->liveData->params.isolationResistanceKOhm );

  // Bms max.regen/power available
  this->spr.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(this->tmpStr1, "xC=%01.00fkW ", this->liveData->params.availableChargePower);
  this->spr.drawString(this->tmpStr1, 192,  zeroY - (maxKw * mulY), 2);
  this->spr.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(this->tmpStr1, "xD=%01.00fkW", this->liveData->params.availableDischargePower);
  this->spr.drawString(this->tmpStr1, 256,  zeroY - (maxKw * mulY), 2);

  //
  this->spr.setTextDatum(TR_DATUM);
  if (this->liveData->params.coolingWaterTempC != -1) {
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "W=%01.00fC" : "W=%01.00fF"), this->liveData->celsius2temperature(this->liveData->params.coolingWaterTempC));
    this->spr.setTextColor(TFT_PURPLE, TFT_TEMP);
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  this->spr.setTextColor(TFT_WHITE, TFT_TEMP);
  if (this->liveData->params.batFanFeedbackHz > 0) {
    sprintf(this->tmpStr1, "FF=%03.00fHz", this->liveData->params.batFanFeedbackHz);
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (this->liveData->params.batFanStatus > 0) {
    sprintf(this->tmpStr1, "FS=%03.00f", this->liveData->params.batFanStatus);
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (this->liveData->params.coolantTemp1C != -1 && this->liveData->params.coolantTemp2C != -1) {
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "C1/2:%01.00f/%01.00fC" : "C1/2:%01.00f/%01.00fF"), this->liveData->celsius2temperature(this->liveData->params.coolantTemp1C), this->liveData->celsius2temperature(this->liveData->params.coolantTemp2C));
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (this->liveData->params.bmsUnknownTempA != -1) {
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "A=%01.00fC" : "W=%01.00fF"), this->liveData->celsius2temperature(this->liveData->params.bmsUnknownTempA));
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (this->liveData->params.bmsUnknownTempB != -1) {
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "B=%01.00fC" : "W=%01.00fF"), this->liveData->celsius2temperature(this->liveData->params.bmsUnknownTempB));
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (this->liveData->params.bmsUnknownTempC != -1) {
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "C=%01.00fC" : "W=%01.00fF"), this->liveData->celsius2temperature(this->liveData->params.bmsUnknownTempC));
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (this->liveData->params.bmsUnknownTempD != -1) {
    sprintf(this->tmpStr1, ((this->liveData->settings.temperatureUnit == 'c') ? "D=%01.00fC" : "W=%01.00fF"), this->liveData->celsius2temperature(this->liveData->params.bmsUnknownTempD));
    this->spr.drawString(this->tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }

  // Print charging time
  time_t diffTime = this->liveData->params.currentTime - this->liveData->params.chargingStartTime;
  if ((diffTime / 60) > 99)
    sprintf(this->tmpStr1, "%02d:%02d:%02d", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
  else
    sprintf(this->tmpStr1, "%02d:%02d", (diffTime / 60), diffTime % 60);
  this->spr.setTextDatum(TL_DATUM);
  this->spr.setTextColor(TFT_SILVER, TFT_BLACK);
  this->spr.drawString(this->tmpStr1, 0, zeroY - (maxKw * mulY), 2);
}

/**
  SOC 10% table (screen 5)
*/
void Board320_240::drawSceneSoc10Table() {

  int zeroY = 2;
  float diffCec, diffCed, diffOdo = -1;
  float firstCed = -1, lastCed = -1, diffCed0to5 = 0;
  float firstCec = -1, lastCec = -1, diffCec0to5 = 0;
  float firstOdo = -1, lastOdo = -1, diffOdo0to5 = 0;
  float diffTime;

  this->spr.setTextSize(1); // Size for small 5x7 font
  this->spr.setTextColor(TFT_SILVER, TFT_TEMP);
  this->spr.setTextDatum(TL_DATUM);
  this->spr.drawString("CONSUMPTION | DISCH.100%->4% SOC",  2, zeroY, 2);

  this->spr.setTextDatum(TR_DATUM);

  this->spr.drawString("dis./char.kWh", 128, zeroY + (1 * 15), 2);
  this->spr.drawString(((this->liveData->settings.distanceUnit == 'k') ? "km" : "mi"), 160, zeroY + (1 * 15), 2);
  this->spr.drawString("kWh100", 224, zeroY + (1 * 15), 2);
  this->spr.drawString("avg.speed", 310, zeroY + (1 * 15), 2);

  for (int i = 0; i <= 10; i++) {
    sprintf(this->tmpStr1, "%d%%", (i == 0) ? 5 : i * 10);
    this->spr.drawString(this->tmpStr1, 32, zeroY + ((12 - i) * 15), 2);

    firstCed = (this->liveData->params.soc10ced[i] != -1) ? this->liveData->params.soc10ced[i] : firstCed;
    lastCed = (lastCed == -1 && this->liveData->params.soc10ced[i] != -1) ? this->liveData->params.soc10ced[i] : lastCed;
    firstCec = (this->liveData->params.soc10cec[i] != -1) ? this->liveData->params.soc10cec[i] : firstCec;
    lastCec = (lastCec == -1 && this->liveData->params.soc10cec[i] != -1) ? this->liveData->params.soc10cec[i] : lastCec;
    firstOdo = (this->liveData->params.soc10odo[i] != -1) ? this->liveData->params.soc10odo[i] : firstOdo;
    lastOdo = (lastOdo == -1 && this->liveData->params.soc10odo[i] != -1) ? this->liveData->params.soc10odo[i] : lastOdo;

    if (i != 10) {
      diffCec = (this->liveData->params.soc10cec[i + 1] != -1 && this->liveData->params.soc10cec[i] != -1) ? (this->liveData->params.soc10cec[i] - this->liveData->params.soc10cec[i + 1]) : 0;
      diffCed = (this->liveData->params.soc10ced[i + 1] != -1 && this->liveData->params.soc10ced[i] != -1) ? (this->liveData->params.soc10ced[i + 1] - this->liveData->params.soc10ced[i]) : 0;
      diffOdo = (this->liveData->params.soc10odo[i + 1] != -1 && this->liveData->params.soc10odo[i] != -1) ? (this->liveData->params.soc10odo[i] - this->liveData->params.soc10odo[i + 1]) : -1;
      diffTime = (this->liveData->params.soc10time[i + 1] != -1 && this->liveData->params.soc10time[i] != -1) ? (this->liveData->params.soc10time[i] - this->liveData->params.soc10time[i + 1]) : -1;
      if (diffCec != 0) {
        sprintf(this->tmpStr1, "+%01.01f", diffCec);
        this->spr.drawString(this->tmpStr1, 128, zeroY + ((12 - i) * 15), 2);
        diffCec0to5 = (i == 0) ? diffCec : diffCec0to5;
      }
      if (diffCed != 0) {
        sprintf(this->tmpStr1, "%01.01f", diffCed);
        this->spr.drawString(this->tmpStr1, 80, zeroY + ((12 - i) * 15), 2);
        diffCed0to5 = (i == 0) ? diffCed : diffCed0to5;
      }
      if (diffOdo != -1) {
        sprintf(this->tmpStr1, "%01.00f", this->liveData->km2distance(diffOdo));
        this->spr.drawString(this->tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
        diffOdo0to5 = (i == 0) ? diffOdo : diffOdo0to5;
        if (diffTime > 0) {
          sprintf(this->tmpStr1, "%01.01f", this->liveData->km2distance(diffOdo) / (diffTime / 3600));
          this->spr.drawString(this->tmpStr1, 310, zeroY + ((12 - i) * 15), 2);
        }
      }
      if (diffOdo > 0 && diffCed != 0) {
        sprintf(this->tmpStr1, "%01.1f", (-diffCed * 100.0 / this->liveData->km2distance(diffOdo)));
        this->spr.drawString(this->tmpStr1, 224, zeroY + ((12 - i) * 15), 2);
      }
    }

    if (diffOdo == -1 && this->liveData->params.soc10odo[i] != -1) {
      sprintf(this->tmpStr1, "%01.00f", this->liveData->km2distance(this->liveData->params.soc10odo[i]));
      this->spr.drawString(this->tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
    }
  }

  this->spr.drawString("0%", 32, zeroY + (13 * 15), 2);
  this->spr.drawString("0-5% is calculated (same) as 5-10%", 310, zeroY + (13 * 15), 2);

  this->spr.drawString("TOT.", 32, zeroY + (14 * 15), 2);
  diffCed = (lastCed != -1 && firstCed != -1) ? firstCed - lastCed + diffCed0to5 : 0;
  sprintf(this->tmpStr1, "%01.01f", diffCed);
  this->spr.drawString(this->tmpStr1, 80, zeroY + (14 * 15), 2);
  diffCec = (lastCec != -1 && firstCec != -1) ? lastCec - firstCec + diffCec0to5 : 0;
  sprintf(this->tmpStr1, "+%01.01f", diffCec);
  this->spr.drawString(this->tmpStr1, 128, zeroY + (14 * 15), 2);
  diffOdo = (lastOdo != -1 && firstOdo != -1) ? lastOdo - firstOdo + diffOdo0to5 : 0;
  sprintf(this->tmpStr1, "%01.00f", this->liveData->km2distance(diffOdo));
  this->spr.drawString(this->tmpStr1, 160, zeroY + (14 * 15), 2);
  sprintf(this->tmpStr1, "AVAIL.CAP: %01.01f kWh", -diffCed - diffCec);
  this->spr.drawString(this->tmpStr1, 310, zeroY + (14 * 15), 2);
}

/**
  DEBUG screen
*/
void Board320_240::drawSceneDebug() {

  int32_t posx, posy;
  String chHex, chHex2;
  uint8_t chByte;

  this->spr.setTextSize(1); // Size for small 5x7 font
  this->spr.setTextColor(TFT_SILVER, TFT_TEMP);
  this->spr.setTextDatum(TL_DATUM);
  this->spr.drawString(debugAtshRequest, 0, 0, 2);
  this->spr.drawString(debugCommandRequest, 128, 0, 2);
  this->spr.drawString(this->liveData->commandRequest, 256, 0, 2);
  this->spr.setTextDatum(TR_DATUM);

  for (int i = 0; i < debugLastString.length() / 2; i++) {
    chHex = debugLastString.substring(i * 2, (i * 2) + 2);
    chHex2 = debugPreviousString.substring(i * 2, (i * 2) + 2);
    this->spr.setTextColor(((chHex.equals(chHex2)) ?  TFT_SILVER : TFT_GREEN), TFT_TEMP);
    chByte = this->liveData->hexToDec(chHex.c_str(), 1, false);
    posx = (((i) % 10) * 32) + 24;
    posy = ((floor((i) / 10)) * 32) + 24;
    sprintf(this->tmpStr1, "%03d", chByte);
    this->spr.drawString(this->tmpStr1, posx + 4, posy, 2);

    this->spr.setTextColor(TFT_YELLOW, TFT_TEMP);
    sprintf(this->tmpStr1, "%c", (char)chByte);
    this->spr.drawString(this->tmpStr1, posx + 4, posy + 13, 2);
  }

  debugPreviousString = debugLastString;
}

/**
   Modify caption
*/
String Board320_240::menuItemCaption(int16_t menuItemId, String title) {

  String prefix = "", suffix = "";

  if (menuItemId == 10) // Version
    suffix = APP_VERSION;

  if (menuItemId == 401) // distance
    suffix = (this->liveData->settings.distanceUnit == 'k') ? "[km]" : "[mi]";
  if (menuItemId == 402) // temperature
    suffix = (this->liveData->settings.temperatureUnit == 'c') ? "[C]" : "[F]";
  if (menuItemId == 403) // pressure
    suffix = (this->liveData->settings.pressureUnit == 'b') ? "[bar]" : "[psi]";

  title = ((prefix == "") ? "" : prefix + " ") + title + ((suffix == "") ? "" : " " + suffix);

  return title;
}

/**
  Display menu
*/
void Board320_240::showMenu() {

  uint16_t posY = 0, tmpCurrMenuItem = 0;

  this->liveData->menuVisible = true;
Serial.println("A");
  this->spr.fillSprite(TFT_BLACK);
  this->spr.setTextDatum(TL_DATUM);
  this->spr.setFreeFont(&Roboto_Thin_24);

Serial.println("B");
  // Page scroll
  uint8_t visibleCount = (int)(this->tft.height() / this->spr.fontHeight());
  if (this->liveData->menuItemSelected >= this->liveData->menuItemOffset + visibleCount)
    this->liveData->menuItemOffset = this->liveData->menuItemSelected - visibleCount + 1;
  if (this->liveData->menuItemSelected < this->liveData->menuItemOffset)
    this->liveData->menuItemOffset = this->liveData->menuItemSelected;
Serial.println("C");

  // Print items
  for (uint16_t i = 0; i < this->liveData->menuItemsCount; ++i) {
    if (this->liveData->menuCurrent == this->liveData->menuItems[i].parentId) {
Serial.println("D");
      if (tmpCurrMenuItem >= this->liveData->menuItemOffset) {
        this->spr.fillRect(0, posY, 320, this->spr.fontHeight() + 2, (this->liveData->menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
        this->spr.setTextColor((this->liveData->menuItemSelected == tmpCurrMenuItem) ? TFT_WHITE : TFT_WHITE, (this->liveData->menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
        this->spr.drawString(this->menuItemCaption(this->liveData->menuItems[i].id, this->liveData->menuItems[i].title), 0, posY + 2, GFXFF);
        posY += this->spr.fontHeight();
      }
      tmpCurrMenuItem++;
    }
  }

  this->spr.pushSprite(0, 0);
}

/**
   Hide menu
*/
void Board320_240::hideMenu() {

  this->liveData->menuVisible = false;
  this->liveData->menuCurrent = 0;
  this->liveData->menuItemSelected = 0;
  this->redrawScreen();
}

/**
  Move in menu with left/right button
*/
void Board320_240::menuMove(bool forward) {

  if (forward) {
    uint16_t tmpCount = 0;
    for (uint16_t i = 0; i < this->liveData->menuItemsCount; ++i) {
      if (this->liveData->menuCurrent == this->liveData->menuItems[i].parentId) {
        tmpCount++;
      }
    }
    this->liveData->menuItemSelected = (this->liveData->menuItemSelected >= tmpCount - 1 ) ? tmpCount - 1 : this->liveData->menuItemSelected + 1;
  } else {
    this->liveData->menuItemSelected = (this->liveData->menuItemSelected <= 0) ? 0 : this->liveData->menuItemSelected - 1;
  }
  this->showMenu();
}

/**
   Enter menu item
*/
void Board320_240::menuItemClick() {

  // Locate menu item for meta data
  MENU_ITEM tmpMenuItem;
  uint16_t tmpCurrMenuItem = 0;
  for (uint16_t i = 0; i < this->liveData->menuItemsCount; ++i) {
    if (this->liveData->menuCurrent == this->liveData->menuItems[i].parentId) {
      if (this->liveData->menuItemSelected == tmpCurrMenuItem) {
        tmpMenuItem = this->liveData->menuItems[i];
        break;
      }
      tmpCurrMenuItem++;
    }
  }

  // Exit menu, parent level menu, open item
  if (this->liveData->menuItemSelected == 0) {
    // Exit menu
    if (tmpMenuItem.parentId == 0 && tmpMenuItem.id == 0) {
      this->liveData->menuVisible = false;
      this->redrawScreen();
    } else {
      // Parent menu
      this->liveData->menuCurrent = tmpMenuItem.targetParentId;
      this->showMenu();
    }
    return;
  } else {
    Serial.println(tmpMenuItem.id);
    // Device list
    if (tmpMenuItem.id > 10000 && tmpMenuItem.id < 10100) {
      strlcpy((char*)this->liveData->settings.obdMacAddress, (char*)tmpMenuItem.obdMacAddress, 20);
      Serial.print("Selected adapter MAC address ");
      Serial.println(this->liveData->settings.obdMacAddress);
      this->saveSettings();
      ESP.restart();
    }
    // Other menus
    switch (tmpMenuItem.id) {
      // Set vehicle type
      case 101: this->liveData->settings.carType = CAR_KIA_ENIRO_2020_64; break;
      case 102: this->liveData->settings.carType = CAR_HYUNDAI_KONA_2020_64; break;
      case 103: this->liveData->settings.carType = CAR_HYUNDAI_IONIQ_2018; break;
      case 104: this->liveData->settings.carType = CAR_KIA_ENIRO_2020_39; break;
      case 105: this->liveData->settings.carType = CAR_HYUNDAI_KONA_2020_39; break;
      case 106: this->liveData->settings.carType = CAR_RENAULT_ZOE; break;
      case 107: this->liveData->settings.carType = CAR_DEBUG_OBD2_KIA; break;
      // Screen orientation
      case 3011: this->liveData->settings.displayRotation = 1; this->tft.setRotation(this->liveData->settings.displayRotation); break;
      case 3012: this->liveData->settings.displayRotation = 3; this->tft.setRotation(this->liveData->settings.displayRotation); break;
      // Default screen
      case 3021: this->liveData->settings.defaultScreen = 1; break;
      case 3022: this->liveData->settings.defaultScreen = 2; break;
      case 3023: this->liveData->settings.defaultScreen = 3; break;
      case 3024: this->liveData->settings.defaultScreen = 4; break;
      case 3025: this->liveData->settings.defaultScreen = 5; break;
      // Debug screen off/on
      case 3031: this->liveData->settings.debugScreen = 0; break;
      case 3032: this->liveData->settings.debugScreen = 1; break;
      // Lcd brightness
      case 3041: this->liveData->settings.lcdBrightness = 0; break;
      case 3042: this->liveData->settings.lcdBrightness = 20; break;
      case 3043: this->liveData->settings.lcdBrightness = 50; break;
      case 3044: this->liveData->settings.lcdBrightness = 100; break;
      // Pre-drawn charg.graphs off/on
      case 3051: this->liveData->settings.predrawnChargingGraphs = 0; break;
      case 3052: this->liveData->settings.predrawnChargingGraphs = 1; break;
      // Distance
      case 4011: this->liveData->settings.distanceUnit = 'k'; break;
      case 4012: this->liveData->settings.distanceUnit = 'm'; break;
      // Temperature
      case 4021: this->liveData->settings.temperatureUnit = 'c'; break;
      case 4022: this->liveData->settings.temperatureUnit = 'f'; break;
      // Pressure
      case 4031: this->liveData->settings.pressureUnit = 'b'; break;
      case 4032: this->liveData->settings.pressureUnit = 'p'; break;
      // Pair ble device
      case 2: scanDevices = true; /*startBleScan(); */return;
      // Reset settings
      case 8: this->resetSettings(); hideMenu(); return;
      // Save settings
      case 9: this->saveSettings(); break;
      // Version
      case 10: hideMenu(); return;
      // Shutdown
      case 11: this->shutdownDevice(); return;
      default:
        // Submenu
        this->liveData->menuCurrent = tmpMenuItem.id;
        this->liveData->menuItemSelected = 0;
        this->showMenu();
        return;
    }

    // close menu
    this->hideMenu();
  }
}

/**
  Redraw screen
*/
void Board320_240::redrawScreen() {

  if (this->liveData->menuVisible) {
    return;
  }

  // Lights not enabled
  if (!this->testDataMode && this->liveData->params.forwardDriveMode && !this->liveData->params.headLights && !this->liveData->params.dayLights) {
    this->spr.fillSprite(TFT_RED);
    this->spr.setFreeFont(&Orbitron_Light_32);
    this->spr.setTextColor(TFT_WHITE, TFT_RED);
    this->spr.setTextDatum(MC_DATUM);
    this->spr.drawString("! LIGHTS OFF !", 160, 120, GFXFF);
    this->spr.pushSprite(0, 0);

    return;
  }

  this->spr.fillSprite(TFT_BLACK);

  // 1. Auto mode = >5kpm Screen 3 - speed, other wise basic Screen2 - Main screen, if charging then Screen 5 Graph
  if (this->displayScreen == SCREEN_AUTO) {
    if (this->liveData->params.speedKmh > 5) {
      if (this->displayScreenAutoMode != 3) {
        this->displayScreenAutoMode = 3;
      }
      this->drawSceneSpeed();
    } else if (this->liveData->params.batPowerKw > 1) {
      if (this->displayScreenAutoMode != 5) {
        this->displayScreenAutoMode = 5;
      }
      this->drawSceneChargingGraph();
    } else {
      if (this->displayScreenAutoMode != 2) {
        this->displayScreenAutoMode = 2;
      }
      this->drawSceneMain();
    }
  }
  // 2. Main screen
  if (this->displayScreen == SCREEN_DASH) {
    this->drawSceneMain();
  }
  // 3. Big speed + kwh/100km
  if (this->displayScreen == SCREEN_SPEED) {
    this->drawSceneSpeed();
  }
  // 4. Battery cells
  if (this->displayScreen == SCREEN_CELLS) {
    this->drawSceneBatteryCells();
  }
  // 5. Charging graph
  if (this->displayScreen == SCREEN_CHARGING) {
    this->drawSceneChargingGraph();
  }
  // 6. SOC10% table (CEC-CED)
  if (this->displayScreen == SCREEN_SOC10) {
    this->drawSceneSoc10Table();
  }
  // 7. DEBUG SCREEN
  if (this->displayScreen == SCREEN_DEBUG) {
    this->drawSceneDebug();
  }

  if (!this->displayScreenSpeedHud) {
    // BLE not connected
    if (!this->liveData->bleConnected && this->liveData->bleConnect) {
      // Print message
      this->spr.setTextSize(1);
      this->spr.setTextColor(TFT_WHITE, TFT_BLACK);
      this->spr.setTextDatum(TL_DATUM);
      this->spr.drawString("BLE4 OBDII not connected...", 0, 180, 2);
      this->spr.drawString("Press middle button to menu.", 0, 200, 2);
      this->spr.drawString(APP_VERSION, 0, 220, 2);
    }

    this->spr.pushSprite(0, 0);
  }
}

/**
   Parse test data
*/
void Board320_240::loadTestData() {

  this->testDataMode = true; // skip lights off message
  this->carInterface->loadTestData();
  this->redrawScreen();
}

/**
   Board looop
*/
void Board320_240::mainLoop() {

  ///////////////////////////////////////////////////////////////////////
  // Handle buttons
  // MIDDLE - menu select
  if (digitalRead(this->pinButtonMiddle) == HIGH) {
    this->btnMiddlePressed = false;
  } else {
    if (!this->btnMiddlePressed) {
      this->btnMiddlePressed = true;
      this->tft.setRotation(this->liveData->settings.displayRotation);
      if (this->liveData->menuVisible) {
        menuItemClick();
      } else {
        this->showMenu();
      }
    }
  }
  // LEFT - screen rotate, menu
  if (digitalRead((this->liveData->settings.displayRotation == 1) ? this->pinButtonRight : this->pinButtonLeft) == HIGH) {
    this->btnLeftPressed = false;
  } else {
    if (!this->btnLeftPressed) {
      this->btnLeftPressed = true;
      this->tft.setRotation(this->liveData->settings.displayRotation);
      // Menu handling
      if (this->liveData->menuVisible) {
        this->menuMove(false);
      } else {
        this->displayScreen++;
        if (this->displayScreen > this->displayScreenCount - (this->liveData->settings.debugScreen == 0) ? 1 : 0)
          this->displayScreen = 0; // rotate screens
        // Turn off display on screen 0
        this->setBrightness((this->displayScreen == SCREEN_BLANK) ? 0 : (this->liveData->settings.lcdBrightness == 0) ? 100 : this->liveData->settings.lcdBrightness);
        this->redrawScreen();
      }
    }
  }
  // RIGHT - menu, debug screen rotation
  if (digitalRead((this->liveData->settings.displayRotation == 1) ? this->pinButtonLeft : this->pinButtonRight) == HIGH) {
    this->btnRightPressed = false;
  } else {
    if (!this->btnRightPressed) {
      this->btnRightPressed = true;
      this->tft.setRotation(this->liveData->settings.displayRotation);
      // Menu handling
      if (this->liveData->menuVisible) {
        this->menuMove(true);
      } else {
        // doAction
        if (this->displayScreen == SCREEN_SPEED) {
          this->displayScreenSpeedHud = !this->displayScreenSpeedHud;
          this->redrawScreen();
        }
        if (this->liveData->settings.debugScreen == 1 && this->displayScreen == SCREEN_DEBUG) {
          this->debugCommandIndex = (this->debugCommandIndex >= this->liveData->commandQueueCount) ? this->liveData->commandQueueLoopFrom : this->debugCommandIndex + 1;
          this->redrawScreen();
        }

      }
    }
  }

}

/**
 * skipAdapterScan
 */
bool Board320_240::skipAdapterScan() {
   return digitalRead(this->pinButtonMiddle) == LOW || digitalRead(this->pinButtonRight) == LOW;
}

#endif // BOARD320_240_CPP
