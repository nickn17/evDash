#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include "config.h"
#include "Board320_240.h"

/**
 * Draws the main screen (screen 1) with live vehicle data.
 *
 * This shows the most important live data values in a grid layout.
 * It includes:
 * - Tire pressures and temperatures
 * - Battery power usage
 * - State of charge percentage
 * - Battery current
 * - Battery voltage
 * - Minimum battery cell voltage
 * - Battery temperature
 * - Auxiliary battery percentage
 * - Auxiliary battery current
 * - Auxiliary battery voltage
 * - Indoor and outdoor temperatures
 */
void Board320_240::drawSceneMain()
{
  // Tire pressure
  char pressureStr[4] = "bar";
  char temperatureStr[2] = "C";
  if (liveData->settings.pressureUnit != 'b')
    strcpy(pressureStr, "psi");
  if (liveData->settings.temperatureUnit != 'c')
    strcpy(temperatureStr, "F");
  if (liveData->params.tireFrontLeftPressureBar == -1)
  {
    sprintf(tmpStr1, "n/a %s", pressureStr);
    sprintf(tmpStr2, "n/a %s", pressureStr);
    sprintf(tmpStr3, "n/a %s", pressureStr);
    sprintf(tmpStr4, "n/a %s", pressureStr);
  }
  else
  {
    sprintf(tmpStr1, "%01.01f%s %02.00f%s", liveData->bar2pressure(liveData->params.tireFrontLeftPressureBar), pressureStr, liveData->celsius2temperature(liveData->params.tireFrontLeftTempC), temperatureStr);
    sprintf(tmpStr2, "%02.00f%s %01.01f%s", liveData->celsius2temperature(liveData->params.tireFrontRightTempC), temperatureStr, liveData->bar2pressure(liveData->params.tireFrontRightPressureBar), pressureStr);
    sprintf(tmpStr3, "%01.01f%s %02.00f%s", liveData->bar2pressure(liveData->params.tireRearLeftPressureBar), pressureStr, liveData->celsius2temperature(liveData->params.tireRearLeftTempC), temperatureStr);
    sprintf(tmpStr4, "%02.00f%s %01.01f%s", liveData->celsius2temperature(liveData->params.tireRearRightTempC), temperatureStr, liveData->bar2pressure(liveData->params.tireRearRightPressureBar), pressureStr);
  }
  showTires(1, 0, 2, 1, tmpStr1, tmpStr2, tmpStr3, tmpStr4, TFT_BLACK);

  // Added later - kwh total in tires box
  // TODO: refactoring
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_GREEN);
  sprSetFont(fontFont2);
  sprintf(tmpStr1, ((liveData->params.cumulativeEnergyChargedKWh == -1) ? "CEC: n/a" : "C: %01.01f +%01.01fkWh"), liveData->params.cumulativeEnergyChargedKWh, liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  sprDrawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 30);
  spr.setTextColor(TFT_YELLOW);
  sprintf(tmpStr1, ((liveData->params.cumulativeEnergyDischargedKWh == -1) ? "CED: n/a" : "D: %01.01f -%01.01fkWh"), liveData->params.cumulativeEnergyDischargedKWh, liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
  sprDrawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 44);

  // batPowerKwh100 on roads, else batPowerAmp
  if (liveData->params.speedKmh > 20 ||
      (liveData->params.speedKmh == -1 && liveData->params.speedKmhGPS > 20 && liveData->params.gpsSat >= 4))
  {
    sprintf(tmpStr1, (liveData->params.batPowerKwh100 == -1 ? "n/a" : "%01.01f"), liveData->km2distance(liveData->params.batPowerKwh100));
    drawBigCell(1, 1, 2, 2, tmpStr1, ((liveData->settings.distanceUnit == 'k') ? "POWER KWH/100KM" : "POWER KWH/100MI"), (liveData->params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }
  else
  {
    // batPowerAmp on chargers (under 10kmh)
    sprintf(tmpStr1, (liveData->params.batPowerKw == -1000) ? "---" : "%01.01f", liveData->params.batPowerKw);
    drawBigCell(1, 1, 2, 2, tmpStr1, "POWER KW", (liveData->params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }

  // socPerc
  sprintf(tmpStr1, (liveData->params.socPerc == -1 ? "n/a" : "%01.00f%%"), liveData->params.socPerc);
  sprintf(tmpStr2, (liveData->params.sohPerc == -1) ? "SOC/SOH?" : (liveData->params.sohPerc == 100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), liveData->params.sohPerc);
  drawBigCell(0, 0, 1, 1, ((liveData->params.socPerc == 255) ? "---" : tmpStr1), tmpStr2,
              (liveData->params.socPerc < 10 || (liveData->params.sohPerc != -1 && liveData->params.sohPerc < 100) ? TFT_RED : (liveData->params.socPerc > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)), TFT_WHITE);

  // batPowerAmp
  sprintf(tmpStr1, (liveData->params.batPowerAmp == -1000) ? "n/a" : (abs(liveData->params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.batPowerAmp);
  drawBigCell(0, 1, 1, 1, tmpStr1, "CURRENT A", (liveData->params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // batVoltage
  sprintf(tmpStr1, (liveData->params.batVoltage == -1) ? "n/a" : "%03.00f", liveData->params.batVoltage);
  drawBigCell(0, 2, 1, 1, tmpStr1, "VOLTAGE", TFT_DEFAULT_BK, TFT_WHITE);

  // batCellMinV
  sprintf(tmpStr1, "%01.02f", liveData->params.batCellMaxV - liveData->params.batCellMinV);
  sprintf(tmpStr2, (liveData->params.batCellMinV == -1) ? "CELLS" : "CELLS %01.02f", liveData->params.batCellMinV);
  drawBigCell(0, 3, 1, 1, (liveData->params.batCellMaxV - liveData->params.batCellMinV == 0.00 ? "OK" : tmpStr1), tmpStr2, TFT_DEFAULT_BK, TFT_WHITE);

  // batTempC
  sprintf(tmpStr1, (liveData->params.batMinC == -100) ? "n/a" : ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), liveData->celsius2temperature(liveData->params.batMinC));
  sprintf(tmpStr2, (liveData->params.batMaxC == -100) ? "BAT.TEMP" : ((liveData->settings.temperatureUnit == 'c') ? "BATT. %01.00fC" : "BATT. %01.01fF"), liveData->celsius2temperature(liveData->params.batMaxC));
  drawBigCell(1, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);

  // batHeaterC
  sprintf(tmpStr1, (liveData->params.batHeaterC == -100) ? "n/a" : ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), liveData->celsius2temperature(liveData->params.batHeaterC));
  drawBigCell(2, 3, 1, 1, tmpStr1, "BAT.HEAT", TFT_TEMP, TFT_WHITE);

  // Aux perc / temp
  if (liveData->settings.carType == CAR_BMW_I3_2014)
  { // TODO: use invalid auxPerc value as decision point here?
    sprintf(tmpStr1, "%01.00f", liveData->params.auxTemperature);
    drawBigCell(3, 0, 1, 1, tmpStr1, "AUX TEMP.", (liveData->params.auxTemperature < 5 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);
  }
  else
  {
    sprintf(tmpStr1, (liveData->params.auxPerc == -1) ? "n/a" : "%01.00f%%", liveData->params.auxPerc);
    drawBigCell(3, 0, 1, 1, tmpStr1, "AUX BAT.", (liveData->params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);
  }

  // Aux amp
  sprintf(tmpStr1, (liveData->params.auxCurrentAmp == -1000) ? "n/a" : (abs(liveData->params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.auxCurrentAmp);
  drawBigCell(3, 1, 1, 1, tmpStr1, "AUX AMPS", (liveData->params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // auxVoltage
  sprintf(tmpStr1, (liveData->params.auxVoltage == -1) ? "n/a" : "%01.01f", liveData->params.auxVoltage);
  drawBigCell(3, 2, 1, 1, tmpStr1, "AUX VOLTS", (liveData->params.auxVoltage < 12.1 ? TFT_RED : (liveData->params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)), TFT_WHITE);

  // indoorTemperature
  sprintf(tmpStr1, (liveData->params.indoorTemperature == -100) ? "n/a" : "%01.01f", liveData->celsius2temperature(liveData->params.indoorTemperature));
  sprintf(tmpStr2, (liveData->params.outdoorTemperature == -100) ? "IN/OUT" : "IN/OUT%01.01f", liveData->celsius2temperature(liveData->params.outdoorTemperature));
  drawBigCell(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, TFT_WHITE);
}

/**
 * Draws the speed and energy usage related screen. (Screen 2)
 *
 * This displays the current speed, average speed, energy usage,
 * odometer, trip meter, and other driving related metrics.
 *
 * It handles formatting and coloring based on value thresholds.
 */
void Board320_240::drawSceneSpeed()
{
  int32_t posx, posy;

  posx = 320 / 2;
  posy = 40;
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(TFT_WHITE);

  // Stopped CAN command queue
  if (liveData->params.stopCommandQueueTime != 0)
  {
    sprintf(tmpStr1, "%s%d", (liveData->params.stopCommandQueue ? "QS " : "QR "), (liveData->params.currentTime - liveData->params.stopCommandQueueTime));
    sprSetFont(fontFont2);
    sprDrawString(tmpStr1, 0, 40);
  }
  if (liveData->params.stopCommandQueue)
  {
    sprSetFont(fontRobotoThin24);
    spr.setTextColor(TFT_RED);
    spr.setTextDatum(TL_DATUM);
    sprDrawString("SENTRY ON", 90, 100);
    sprSetFont(fontFont2);
    spr.setTextColor(TFT_WHITE);
    spr.setTextDatum(TL_DATUM);
    sprintf(tmpStr1, "GPSw:%u GYw:%u", liveData->params.gpsWakeCount, liveData->params.gyroWakeCount);
    sprDrawString(tmpStr1, 90, 130);
    if (liveData->params.speedKmhGPS < 0)
    {
      snprintf(tmpStr2, sizeof(tmpStr2), "GPS:-- SAT:%u", liveData->params.gpsSat);
    }
    else
    {
      snprintf(tmpStr2, sizeof(tmpStr2), "GPS:%03.0f SAT:%u", liveData->params.speedKmhGPS, liveData->params.gpsSat);
    }
    sprDrawString(tmpStr2, 90, 145);
    bool auxFresh = false;
    if (liveData->params.auxVoltage != -1 && liveData->params.currentTime != 0)
    {
      if (liveData->settings.voltmeterEnabled == 1)
      {
        const time_t auxFreshWindowInaSec = 15;
        auxFresh = (liveData->params.currentTime - liveData->params.lastVoltageReadTime <= auxFreshWindowInaSec);
      }
      else
      {
        const time_t auxFreshWindowCanSec = 5;
        auxFresh = (liveData->params.currentTime - liveData->params.lastCanbusResponseTime <= auxFreshWindowCanSec);
      }
    }
    if (!auxFresh)
    {
      snprintf(tmpStr3, sizeof(tmpStr3), "AUX: --.-V");
    }
    else
    {
      snprintf(tmpStr3, sizeof(tmpStr3), "AUX:%04.1fV", liveData->params.auxVoltage);
    }
    sprDrawString(tmpStr3, 90, 160);
    if (liveData->params.motionWakeLocked)
    {
      sprDrawString("Touch to wake", 90, 175);
    }
    return;
  }

  // Speed or charging data
  if (liveData->params.chargingOn)
  {
    // Charging voltage, current, time
    posy = 40;
    sprSetFont(fontRobotoThin24);
    spr.setTextDatum(TR_DATUM); // Top center
    spr.setTextColor(TFT_WHITE);
    time_t diffTime = liveData->params.currentTime - liveData->params.chargingStartTime;
    if ((diffTime / 60) > 99)
      sprintf(tmpStr3, "%02ld:%02ld:%02ld", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
    else
      sprintf(tmpStr3, "%02ld:%02ld", (diffTime / 60), diffTime % 60);
    sprDrawString(tmpStr3, 200, posy);
    posy += 24;
    sprintf(tmpStr3, (liveData->params.batVoltage == -1000) ? "n/a V" : "%01.01f V", liveData->params.batVoltage);
    sprDrawString(tmpStr3, 200, posy);
    posy += 24;
    sprintf(tmpStr3, (liveData->params.batPowerAmp == -1000) ? "n/a A" : "%01.01f A", liveData->params.batPowerAmp);
    sprDrawString(tmpStr3, 200, posy);
    posy += 24;
    if (diffTime > 5)
    {
      sprintf(tmpStr3, "avg.%01.01f kW", ((liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart) * (3600 / diffTime)));
      sprDrawString(tmpStr3, 200, posy);
    }
  }
  else
  {
    // Speed
    spr.setTextSize(2);
    sprintf(tmpStr3, "%01.00f", liveData->km2distance((((liveData->params.speedKmhGPS > 10 && liveData->settings.carSpeedType == CAR_SPEED_TYPE_AUTO) || liveData->settings.carSpeedType == CAR_SPEED_TYPE_GPS) ? liveData->params.speedKmhGPS : ((((liveData->params.speedKmh > 10 && liveData->settings.carSpeedType == CAR_SPEED_TYPE_AUTO) || liveData->settings.carSpeedType == CAR_SPEED_TYPE_CAR)) ? liveData->params.speedKmh : 0))));
    sprSetFont(fontFont7);
    sprDrawString(tmpStr3, 200, posy);
  }

  posy = 140;
  spr.setTextDatum(TR_DATUM); // Top center
  spr.setTextSize(1);
  if ((liveData->params.speedKmh > 25 || (liveData->params.speedKmhGPS > 25 && liveData->params.gpsSat >= 4)) && liveData->params.batPowerKw < 0)
  {
    sprintf(tmpStr3, (liveData->params.batPowerKwh100 == -1000) ? "n/a" : "%01.01f", liveData->km2distance(liveData->params.batPowerKwh100));
    sprSetFont(fontFont2);
    sprDrawString("kWh/100km", 200, posy + 46);
  }
  else
  {
    sprintf(tmpStr3, (liveData->params.batPowerKw == -1000) ? "n/a" : "%01.01f", liveData->params.batPowerKw);
    sprSetFont(fontFont2);
    sprDrawString("kW", 200, posy + 48);
  }
  sprSetFont(fontFont7);
  sprDrawString(tmpStr3, 200, posy);

  // Bottom 2 numbers with charged/discharged kWh from start
  sprSetFont(fontRobotoThin24);
  spr.setTextColor(TFT_WHITE);
  posx = 0;
  posy = 0;
  spr.setTextDatum(TL_DATUM);
  sprintf(tmpStr3, (liveData->params.odoKm == -1) ? "n/a km" : ((liveData->settings.distanceUnit == 'k') ? "%01.00fkm" : "%01.00fmi"), liveData->km2distance(liveData->params.odoKm));
  sprDrawString(tmpStr3, posx, posy);
  sprintf(tmpStr3, "N");
  if (liveData->params.forwardDriveMode)
  {
    if (liveData->params.odoKm > 0 && liveData->params.odoKmStart > 0 &&
        (liveData->params.odoKm - liveData->params.odoKmStart) > 0 && liveData->params.speedKmhGPS < 150)
    {
      sprintf(tmpStr3, ((liveData->settings.distanceUnit == 'k') ? "%01.00f" : "%01.00f"), liveData->km2distance(liveData->params.odoKm - liveData->params.odoKmStart));
    }
    else
    {
      sprintf(tmpStr3, "D");
    }
  }
  else if (liveData->params.reverseDriveMode)
  {
    sprintf(tmpStr3, "R");
  }
  if (liveData->params.chargingOn)
  {
    sprintf(tmpStr3, "AC/DC");
  }
  if (liveData->params.chargerACconnected)
  {
    sprintf(tmpStr3, "AC");
  }
  if (liveData->params.chargerDCconnected)
  {
    sprintf(tmpStr3, "DC");
  }
  sprDrawString(tmpStr3, posx, posy + 20);

  spr.setTextDatum(TR_DATUM);
  if (liveData->params.batteryManagementMode != BAT_MAN_MODE_NOT_IMPLEMENTED)
  {
    uint16_t bmsStateColor = TFT_WHITE;
    if (liveData->params.batteryManagementMode == BAT_MAN_MODE_PTC_HEATER)
      bmsStateColor = TFT_RED;
    else if (liveData->params.batteryManagementMode == BAT_MAN_MODE_COOLING ||
             liveData->params.batteryManagementMode == BAT_MAN_MODE_LOW_TEMPERATURE_RANGE ||
             liveData->params.batteryManagementMode == BAT_MAN_MODE_LOW_TEMPERATURE_RANGE_COOLING)
      bmsStateColor = TFT_CYAN;
    spr.setTextColor(bmsStateColor);
    sprintf(tmpStr1, "%s %01.00f", liveData->getBatteryManagementModeStr(liveData->params.batteryManagementMode).c_str(),
            liveData->celsius2temperature(liveData->params.coolingWaterTempC));
    sprDrawString(tmpStr1, 319 - posx, posy);
    spr.setTextColor(TFT_WHITE);
  }

  // Avg speed
  posy = 60;
  sprintf(tmpStr3, "%01.00f", liveData->params.avgSpeedKmh);
  spr.setTextDatum(TL_DATUM);
  sprDrawString(tmpStr3, posx, posy);
  sprSetFont(fontFont2);
  sprDrawString("avg.km/h", 0, posy + 20);
  posy += 40;

  // AUX voltage
  if (liveData->params.auxVoltage > 5 && liveData->params.speedKmh < 100 && liveData->params.speedKmhGPS < 100)
  {
    if (liveData->params.auxPerc != -1)
    {
      sprintf(tmpStr3, "%01.00f", liveData->params.auxPerc);
      spr.setTextDatum(TL_DATUM);
      sprSetFont(fontRobotoThin24);
      sprDrawString(tmpStr3, posx, posy);
      sprSetFont(fontFont2);
      sprDrawString("aux %", 0, posy + 20);
      posy += 40;
    }
    sprintf(tmpStr3, "%01.01f", liveData->params.auxVoltage);
    spr.setTextDatum(TL_DATUM);
    sprSetFont(fontRobotoThin24);
    sprDrawString(tmpStr3, posx, posy);
    sprSetFont(fontFont2);
    sprDrawString("aux V", 0, posy + 20);
    posy += 40;
  }

  // Bottom info
  // Cummulative regen/power
  posy = 240;
  sprintf(tmpStr3, "-%01.01f +%01.01f", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart,
          liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.setTextDatum(BL_DATUM);
  sprSetFont(fontRobotoThin24);
  sprDrawString(tmpStr3, posx, posy);
  sprSetFont(fontFont2);
  sprDrawString("cons./regen.kWh", 0, 240 - 28);
  posx = 319;
  float kwh100a = 0;
  float kwh100b = 0;
  if (liveData->params.odoKm != -1 && liveData->params.odoKm != -1 && liveData->params.odoKm != liveData->params.odoKmStart)
    kwh100a = ((100 * ((liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart) -
                       (liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart))) /
               (liveData->params.odoKm - liveData->params.odoKmStart));
  if (liveData->params.odoKm != -1 && liveData->params.odoKm != -1 && liveData->params.odoKm != liveData->params.odoKmStart)
    kwh100b = ((100 * ((liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart))) /
               (liveData->params.odoKm - liveData->params.odoKmStart));
  sprintf(tmpStr3, "%01.01f/%01.01f", kwh100a, kwh100b);
  spr.setTextDatum(BR_DATUM);
  sprSetFont(fontFont2);
  sprDrawString("avg.kWh/100km", posx, 240 - 28);
  sprSetFont(fontRobotoThin24);
  sprDrawString(tmpStr3, posx, posy);
  // Bat.power
  /*posx = 320 / 2;
  sprintf(tmpStr3, (liveData->params.batPowerKw == -1000) ? "n/a kw" : "%01.01fkw", liveData->params.batPowerKw);
  spr.setTextDatum(BC_DATUM);
  sprDrawString(tmpStr3, posx, posy);*/

  // RIGHT INFO
  // Battery "cold gate" detection
  // Dashed separators around min/max cell block.
  // Top dashed line animates ("marching") when PTC heater mode is active.
  const int16_t sepX = 210;
  const int16_t sepW = 110;
  const int16_t sepTopY = 55;
  const int16_t sepBottomY = 120;
  const int16_t sepH = 5;
  const int16_t dashLen = 8;
  const int16_t gapLen = 4;
  const int16_t dashPeriod = dashLen + gapLen;
  const uint16_t topSepColor = (liveData->params.batMaxC >= 15) ? ((liveData->params.batMaxC >= 25) ? ((liveData->params.batMaxC >= 35) ? TFT_YELLOW : TFT_DARKGREEN2) : TFT_BLUE) : TFT_RED;
  const uint16_t bottomSepColor = (liveData->params.batMinC >= 15) ? ((liveData->params.batMinC >= 25) ? ((liveData->params.batMinC >= 35) ? TFT_YELLOW : TFT_DARKGREEN2) : TFT_BLUE) : TFT_RED;
  const bool ptcActive = (liveData->params.batteryManagementMode == BAT_MAN_MODE_PTC_HEATER);
  const int16_t topDashOffset = ptcActive ? int16_t((millis() / 120) % dashPeriod) : 0;

  for (int16_t x = sepX - topDashOffset; x < sepX + sepW; x += dashPeriod)
  {
    int16_t drawX = (x < sepX) ? sepX : x;
    int16_t drawW = x + dashLen - drawX;
    if (drawX + drawW > sepX + sepW)
      drawW = (sepX + sepW) - drawX;
    if (drawW > 0)
      spr.fillRect(drawX, sepTopY, drawW, sepH, topSepColor);
  }
  for (int16_t x = sepX; x < sepX + sepW; x += dashPeriod)
  {
    int16_t drawW = dashLen;
    if (x + drawW > sepX + sepW)
      drawW = (sepX + sepW) - x;
    if (drawW > 0)
      spr.fillRect(x, sepBottomY, drawW, sepH, bottomSepColor);
  }
  spr.setTextColor(TFT_WHITE);
  sprSetFont(fontRobotoThin24);
  spr.setTextDatum(TR_DATUM);
  sprintf(tmpStr3, (liveData->params.batMaxC == -100) ? "-" : "%01.00f", liveData->celsius2temperature(liveData->params.batMaxC));
  sprDrawString(tmpStr3, 319, 66);
  sprintf(tmpStr3, (liveData->params.batMinC == -100) ? "-" : "%01.00f", liveData->celsius2temperature(liveData->params.batMinC));
  sprDrawString(tmpStr3, 319, 92);
  if (liveData->params.motor1Rpm > 0 || liveData->params.motor2Rpm > 0)
  {
    sprintf(tmpStr3, "%01.01f/%01.01fkr", (liveData->params.motor1Rpm / 1000), (liveData->params.motor2Rpm / 1000));
    sprDrawString(tmpStr3, 319, 26);
  }
  else if (liveData->params.outdoorTemperature != -100)
  {
    sprintf(tmpStr3, "out %01.01f", liveData->celsius2temperature(liveData->params.outdoorTemperature)); //, liveData->celsius2temperature(liveData->params.motorTempC));
    sprDrawString(tmpStr3, 319, 26);
  }

  // Min.Cell V
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor((liveData->params.batCellMinV > 1.5 && liveData->params.batCellMinV < 3.0) ? TFT_RED : TFT_WHITE);
  sprintf(tmpStr3, (liveData->params.batCellMaxV == -1) ? "n/a V" : "%01.02fV", liveData->params.batCellMaxV);
  sprDrawString(tmpStr3, 280, 66);
  spr.setTextColor((liveData->params.batCellMinV > 1.5 && liveData->params.batCellMinV < 3.0) ? TFT_RED : TFT_WHITE);
  sprintf(tmpStr3, (liveData->params.batCellMinV == -1) ? "n/a V" : "%01.02fV", liveData->params.batCellMinV);
  sprDrawString(tmpStr3, 280, 92);

  // Rear car + brake lights
  {
    const int16_t rearW = 70;
    const int16_t rearH = 22;
    const int16_t rearX = (320 - rearW) / 2;
    const int16_t rearY = 240 - rearH;
    const uint16_t bodyColor = 0x4208;
    const uint16_t lineColor = 0x5AEB;
    const uint16_t lightOn = TFT_RED;
    const uint16_t lightOff = TFT_DARKRED;
    const bool brakeOn = liveData->params.brakeLights;
    const uint16_t lightColor = brakeOn ? lightOn : lightOff;
    const int16_t lightMargin = brakeOn ? 4 : 6;
    const int16_t lightW = brakeOn ? 16 : 12;
    const int16_t lightH = brakeOn ? 8 : 6;
    const int16_t lightY = rearY + (brakeOn ? 3 : 4);
    const int16_t lightRadius = brakeOn ? 3 : 2;
    const int16_t leftLightX = rearX + lightMargin;
    const int16_t rightLightX = rearX + rearW - lightMargin - lightW;

    spr.fillRoundRect(rearX, rearY, rearW, rearH, 6, bodyColor);
    spr.drawRoundRect(rearX, rearY, rearW, rearH, 6, lineColor);
    spr.fillRoundRect(rearX + 8, rearY + 4, rearW - 16, 6, 3, lineColor);
    spr.drawFastHLine(rearX + 6, rearY + rearH - 6, rearW - 12, lineColor);
    spr.fillRoundRect(leftLightX, lightY, lightW, lightH, lightRadius, lightColor);
    spr.fillRoundRect(rightLightX, lightY, lightW, lightH, lightRadius, lightColor);
    if (brakeOn)
    {
      spr.fillRoundRect(leftLightX + 2, lightY + 2, lightW - 4, lightH - 4, 2, TFT_ORANGE);
      spr.fillRoundRect(rightLightX + 2, lightY + 2, lightW - 4, lightH - 4, 2, TFT_ORANGE);
    }
  }
  // Lights
  uint16_t tmpWord;
  tmpWord = (liveData->params.headLights) ? TFT_GREEN : (liveData->params.autoLights) ? TFT_YELLOW
                                                    : (liveData->params.dayLights)    ? TFT_BLUE
                                                                                      : TFT_BLACK;
  spr.fillRect(145, 26, 20, 4, tmpWord);
  spr.fillRect(170, 26, 20, 4, tmpWord);

  // Soc%, bat.kWh
  spr.setTextColor((liveData->params.socPerc <= 15) ? TFT_RED : (liveData->params.socPerc > 80 || (liveData->params.socPerc == -1 && liveData->params.socPerc > 80)) ? TFT_YELLOW
                                                                                                                                                                     : TFT_GREEN);
  spr.setTextDatum(BR_DATUM);
  sprintf(tmpStr3, (liveData->params.socPerc == -1) ? "n/a" : "%01.00f", liveData->params.socPerc);
  sprSetFont(fontOrbitronLight32);
  sprDrawString(tmpStr3, 285, 165);
  sprSetFont(fontOrbitronLight24);
  sprDrawString("%", 319, 155);
  if (liveData->params.socPerc > 0)
  {
    float capacity = liveData->params.batteryTotalAvailableKWh * (liveData->params.socPerc / 100);
    // calibration for Niro/Kona, real available capacity is ~66.5kWh, 0-10% ~6.2kWh, 90-100% ~7.2kWh
    if (liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64 || liveData->settings.carType == CAR_KIA_ESOUL_2020_64)
    {
      capacity = (liveData->params.socPerc * 0.615) * (1 + (liveData->params.socPerc * 0.0008));
    }
    spr.setTextColor(TFT_WHITE);
    sprSetFont(fontOrbitronLight32);
    sprintf(tmpStr3, "%01.00f", capacity);
    sprDrawString(tmpStr3, 285, 200);
    sprSetFont(fontOrbitronLight24);
    sprintf(tmpStr3, ".%d", int(10 * (capacity - (int)capacity)));
    sprDrawString(tmpStr3, 319, 200);
    spr.setTextColor(TFT_SILVER);
    sprSetFont(fontFont2);
    sprDrawString("kWh", 319, 174);
  }
}

/**
 * Draws the heads-up display (HUD) scene on the board's 320x240 display.
 *
 * The HUD shows key driving information like speed, power, SOC. It is
 * optimized for vertical orientation while driving.
 */
void Board320_240::drawSceneHud()
{
  float batColor;

  // Change rotation to vertical & mirror
  if (tft.getRotation() != 7)
  {
    tft.setRotation(7);
    tft.fillScreen(TFT_BLACK);
  }

  if (liveData->commConnected && firstReload < 3)
  {
    tft.fillScreen(TFT_BLACK);
    firstReload++;
  }

  tft.setTextDatum(TR_DATUM);             // top-right alignment
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // foreground, background text color

  // Draw speed
  tft.setTextSize(3);
  sprintf(tmpStr3, "0");
    if (liveData->params.speedKmhGPS > 10 && liveData->params.gpsSat >= 4)
  {
    tft.fillRect(0, 210, 320, 30, TFT_BLACK);
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmhGPS));
    lastSpeedKmh = liveData->params.speedKmhGPS;
  }
  else if (liveData->params.speedKmh > 10)
  {
    if (liveData->params.speedKmh != lastSpeedKmh)
    {
      tft.fillRect(0, 210, 320, 30, TFT_BLACK);
      sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.speedKmh));
      lastSpeedKmh = liveData->params.speedKmh;
    }
  }
  else
  {
    sprintf(tmpStr3, "0");
  }
  tft.setFont(fontFont7);
  tftDrawStringFont7(tmpStr3, 319, 0);

  // Draw power kWh/100km (>25kmh) else kW
  tft.setTextSize(1);
  if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0)
  {
    sprintf(tmpStr3, "%01.00f", liveData->km2distance(liveData->params.batPowerKwh100));
  }
  else
  {
    sprintf(tmpStr3, "%01.01f", liveData->params.batPowerKw);
  }
  tft.fillRect(181, 149, 150, 50, TFT_BLACK);
  tft.setFont(fontFont7);
  tftDrawStringFont7(tmpStr3, 320, 150);

  // Draw soc%
  sprintf(tmpStr3, "%01.00f%c", liveData->params.socPerc, '%');
  tft.setFont(fontFont7);
  tftDrawStringFont7(tmpStr3, 160, 150);

  // Cold gate battery
  batColor = (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED;
  tft.fillRect(0, 70, 50, 140, batColor);
  tft.fillRect(15, 60, 20, 10, batColor);
  tft.setTextColor(TFT_WHITE, batColor);
  tft.setFont(fontRobotoThin24);
  tft.setTextDatum(MC_DATUM);
  sprintf(tmpStr3, "%01.00f", liveData->celsius2temperature(liveData->params.batTempC));
  tft.drawString(tmpStr3, 25, 180);

  // Rear car + brake lights
  {
    const int16_t rearW = 70;
    const int16_t rearH = 22;
    const int16_t rearX = (320 - rearW) / 2;
    const int16_t rearY = 240 - rearH;
    const uint16_t bodyColor = 0x4208;
    const uint16_t lineColor = 0x5AEB;
    const uint16_t lightOn = TFT_RED;
    const uint16_t lightOff = TFT_DARKRED;
    const bool brakeOn = liveData->params.brakeLights;
    const uint16_t lightColor = brakeOn ? lightOn : lightOff;
    const int16_t lightMargin = brakeOn ? 4 : 6;
    const int16_t lightW = brakeOn ? 16 : 12;
    const int16_t lightH = brakeOn ? 8 : 6;
    const int16_t lightY = rearY + (brakeOn ? 3 : 4);
    const int16_t lightRadius = brakeOn ? 3 : 2;
    const int16_t leftLightX = rearX + lightMargin;
    const int16_t rightLightX = rearX + rearW - lightMargin - lightW;

    tft.fillRoundRect(rearX, rearY, rearW, rearH, 6, bodyColor);
    tft.drawRoundRect(rearX, rearY, rearW, rearH, 6, lineColor);
    tft.fillRoundRect(rearX + 8, rearY + 4, rearW - 16, 6, 3, lineColor);
    tft.drawFastHLine(rearX + 6, rearY + rearH - 6, rearW - 12, lineColor);
    tft.fillRoundRect(leftLightX, lightY, lightW, lightH, lightRadius, lightColor);
    tft.fillRoundRect(rightLightX, lightY, lightW, lightH, lightRadius, lightColor);
    if (brakeOn)
    {
      tft.fillRoundRect(leftLightX + 2, lightY + 2, lightW - 4, lightH - 4, 2, TFT_ORANGE);
      tft.fillRoundRect(rightLightX + 2, lightY + 2, lightW - 4, lightH - 4, 2, TFT_ORANGE);
    }
  }
}

/**
 * Number of visible battery cell values per page.
 */
uint16_t Board320_240::batteryCellsCellsPerPage()
{
  int32_t lastPosY = 64 - 16;
  if (liveData->params.batModuleTempCount > 4)
  {
    uint16_t moduleRows = (liveData->params.batModuleTempCount + 7) / 8;
    lastPosY = ((moduleRows - 1) * 13) + 32;
  }

  int32_t firstCellY = lastPosY + 16;
  int32_t availableHeight = 240 - firstCellY;
  uint16_t rowsPerPage = (availableHeight > 0) ? (availableHeight / 13) : 0;
  if (rowsPerPage < 1)
    rowsPerPage = 1;

  return rowsPerPage * 8;
}

/**
 * Number of pages for battery cell values.
 */
uint16_t Board320_240::batteryCellsPageCount()
{
  uint16_t cellsPerPage = batteryCellsCellsPerPage();
  uint16_t cellCount = liveData->params.cellCount;
  if (cellsPerPage == 0 || cellCount == 0)
    return 1;

  uint16_t pageCount = (cellCount + cellsPerPage - 1) / cellsPerPage;
  return (pageCount == 0) ? 1 : pageCount;
}

/**
 * Move battery cells page.
 */
void Board320_240::batteryCellsPageMove(bool forward)
{
  uint16_t pageCount = batteryCellsPageCount();
  if (pageCount <= 1)
  {
    batteryCellsPage = 0;
    return;
  }

  if (forward)
    batteryCellsPage = (batteryCellsPage + 1 >= pageCount) ? 0 : (batteryCellsPage + 1);
  else
    batteryCellsPage = (batteryCellsPage == 0) ? (pageCount - 1) : (batteryCellsPage - 1);

  redrawScreen();
}

/**
 * Draws the battery cells screen. (Screen 3)
 *
 * Displays the heater, inlet, and module temperatures.
 * Then displays the voltage for each cell.
 * Cell colors indicate temperature or min/max voltage.
 */
void Board320_240::drawSceneBatteryCells()
{
  int32_t posx, posy;
  int32_t lastPosY = 64 - 16;

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batHeaterC), char(127));
  drawSmallCell(0, 0, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batInletC), char(127));
  drawSmallCell(1, 0, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);

  if (liveData->params.batModuleTempCount <= 4)
  {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[0]), char(127));
    drawSmallCell(0, 1, 1, 1, tmpStr1, "MO1", TFT_TEMP, (liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[1]), char(127));
    drawSmallCell(1, 1, 1, 1, tmpStr1, "MO2", TFT_TEMP, (liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[2]), char(127));
    drawSmallCell(2, 1, 1, 1, tmpStr1, "MO3", TFT_TEMP, (liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[3]), char(127));
    drawSmallCell(3, 1, 1, 1, tmpStr1, "MO4", TFT_TEMP, (liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  }
  else
  {
    // Ioniq (up to 12 cells)
    for (uint16_t i = 0; i < liveData->params.batModuleTempCount; i++)
    {
      if (/*liveData->params.batModuleTempC[i] == 0 && */ liveData->params.batModuleTempC[i] == -100)
        continue;
      posx = (((i - 0) % 8) * 40);
      posy = lastPosY = ((floor((i - 0) / 8)) * 13) + 32;
      // spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
      spr.setTextSize(1); // Size for small 5x7 font
      spr.setTextDatum(TL_DATUM);
      spr.setTextColor(((liveData->params.batModuleTempC[i] >= 15) ? ((liveData->params.batModuleTempC[i] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED));
      sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"), liveData->celsius2temperature(liveData->params.batModuleTempC[i]), char(127));
      sprSetFont(fontFont2);
      sprDrawString(tmpStr1, posx + 4, posy);
    }
  }

  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextSize(1);         // Size for small 5x7 font

  uint16_t cellsPerPage = batteryCellsCellsPerPage();
  uint16_t pageCount = batteryCellsPageCount();
  if (batteryCellsPage >= pageCount)
    batteryCellsPage = pageCount - 1;

  uint16_t cellFirstIndex = batteryCellsPage * cellsPerPage;
  uint16_t cellLastIndex = cellFirstIndex + cellsPerPage;
  if (cellLastIndex > liveData->params.cellCount)
    cellLastIndex = liveData->params.cellCount;

  if (pageCount > 1)
  {
    spr.setTextDatum(TR_DATUM);
    spr.setTextColor(TFT_CYAN);
    sprSetFont(fontFont2);
    sprintf(tmpStr4, "<PG %d/%d>", batteryCellsPage + 1, pageCount);
    sprDrawString(tmpStr4, 318, 18);
    spr.setTextDatum(TL_DATUM);
  }

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < liveData->params.cellCount; i++)
  {
    if ((liveData->params.cellVoltage[i] < minVal || minVal == -1) && liveData->params.cellVoltage[i] != -1)
      minVal = liveData->params.cellVoltage[i];
    if ((liveData->params.cellVoltage[i] > maxVal || maxVal == -1) && liveData->params.cellVoltage[i] != -1)
      maxVal = liveData->params.cellVoltage[i];
    /*if (liveData->params.cellVoltage[i] > 0 && i > liveData->params.cellCount + 1)
      liveData->params.cellCount = i + 1;*/
  }

  // Draw cell matrix
  for (int i = cellFirstIndex; i < cellLastIndex; i++)
  {
    if (liveData->params.cellVoltage[i] == -1)
      continue;
    int localIndex = i - cellFirstIndex;
    posx = ((localIndex % 8) * 40) + 4;
    posy = ((localIndex / 8) * 13) + lastPosY + 16;
    sprintf(tmpStr3, "%01.02f", liveData->params.cellVoltage[i]);
    spr.setTextColor(TFT_SILVER);
    if (liveData->params.cellVoltage[i] == minVal && minVal != maxVal)
      spr.setTextColor(TFT_RED);
    if (liveData->params.cellVoltage[i] == maxVal && minVal != maxVal)
      spr.setTextColor(TFT_GREEN);
    // Battery cell imbalance detetection
    if (liveData->params.cellVoltage[i] > 1.5 && liveData->params.cellVoltage[i] < 3.0)
      spr.setTextColor(TFT_WHITE, TFT_RED);
    sprSetFont(fontFont2);
    sprDrawString(tmpStr3, posx, posy);
  }
}

/**
 * Charging graph (Screen 4)
 */
void Board320_240::drawSceneChargingGraph()
{
  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3;   // 100% = 300px
  float mulY = 2; // 100kW = 200px
  int maxKw = 75;
  uint16_t color;

  // Determine the maximum kW value to scale the Y-axis properly
  for (int i = 0; i <= 100; i++)
  {
    if (liveData->params.chargingGraphMaxKw[i] > maxKw)
    {
      maxKw = liveData->params.chargingGraphMaxKw[i];
    }
  }

  // Round up maxKw to the next multiple of 10
  maxKw = ((maxKw + 9) / 10) * 10;
  maxKw = 200;
  // Recalculate the Y-axis multiplier based on the actual maxKw
  mulY = 160.0f / maxKw;

  spr.fillSprite(TFT_BLACK);

  // Display basic information cells
  sprintf(tmpStr1, "%01.00f", liveData->params.socPerc);
  drawSmallCell(0, 0, 1, 1, tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerKw);
  drawSmallCell(1, 0, 1, 1, tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerAmp);
  drawSmallCell(2, 0, 1, 1, tmpStr1, "CURRENT A", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, "%03.00f", liveData->params.batVoltage);
  drawSmallCell(3, 0, 1, 1, tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  // Temperature related cells
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"),
          liveData->celsius2temperature(liveData->params.batHeaterC), char(127));
  drawSmallCell(0, 1, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_RED);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"),
          liveData->celsius2temperature(liveData->params.batInletC), char(127));
  drawSmallCell(1, 1, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"),
          liveData->celsius2temperature(liveData->params.batMinC), char(127));
  drawSmallCell(2, 1, 1, 1, tmpStr1, "BAT.MIN", (liveData->params.batMinC >= 15) ? ((liveData->params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f%cC" : "%01.01f%cF"),
          liveData->celsius2temperature(liveData->params.outdoorTemperature), char(127));
  drawSmallCell(3, 1, 1, 1, tmpStr1, "OUT.TEMP.", TFT_TEMP, TFT_CYAN);

  spr.setTextColor(TFT_SILVER);
  sprSetFont(fontFont2);

  // Draw vertical grid lines every 10% (X-axis)
  for (int i = 0; i <= 100; i += 10)
  {
    color = (i == 0 || i == 50 || i == 100) ? TFT_DARKRED : TFT_DARKRED2;
    spr.drawFastVLine(zeroX + (i * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
  }

  // Draw horizontal grid lines every 10kW (Y-axis)
  spr.setTextDatum(ML_DATUM);
  for (int i = 0; i <= maxKw; i += (maxKw > 150 ? 20 : 10))
  {
    color = ((i % 50) == 0 || i == 0) ? TFT_DARKRED : TFT_DARKRED2;
    spr.drawFastHLine(zeroX, zeroY - (i * mulY), 100 * mulX, color);

    sprintf(tmpStr1, "%d", i);
    sprDrawString(tmpStr1, zeroX + (100 * mulX) + (i > 100 ? 0 : 3), zeroY - (i * mulY));
  }

  // Draw real-time values (temperature and kW lines)
  for (int i = 0; i <= 100; i++)
  {
    if (liveData->params.chargingGraphBatMinTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMinTempC[i] * mulY), mulX, TFT_BLUE);

    if (liveData->params.chargingGraphBatMaxTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMaxTempC[i] * mulY), mulX, TFT_BLUE);

    if (liveData->params.chargingGraphWaterCoolantTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphWaterCoolantTempC[i] * mulY), mulX, TFT_PURPLE);

    if (liveData->params.chargingGraphHeaterTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphHeaterTempC[i] * mulY), mulX, TFT_RED);

    if (liveData->params.chargingGraphMinKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMinKw[i] * mulY), mulX, TFT_GREENYELLOW);

    if (liveData->params.chargingGraphMaxKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMaxKw[i] * mulY), mulX, TFT_YELLOW);
  }

  // Print the charging time
  time_t diffTime = liveData->params.currentTime - liveData->params.chargingStartTime;
  if ((diffTime / 60) > 99)
    sprintf(tmpStr1, "%02ld:%02ld:%02ld", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
  else
    sprintf(tmpStr1, "%02ld:%02ld", (diffTime / 60), diffTime % 60);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER);
  sprDrawString(tmpStr1, 0, zeroY - (maxKw * mulY));
}
/**
 * Draws the SOC 10% table screen. (screen 5)
 *
 * Displays discharge consumption and characteristics from 100% to 4% SOC,
 * in 10% increments. Calculates differences between SOC levels for discharge
 * energy consumed, charge energy consumed, odometer distance, and speed.
 * Prints totals for 0-100% and estimated available capacity.
 */
void Board320_240::drawSceneSoc10Table()
{
  int zeroY = 2;
  float diffCec, diffCed, diffOdo = -1;
  float firstCed = -1, lastCed = -1, diffCed0to5 = 0;
  float firstCec = -1, lastCec = -1, diffCec0to5 = 0;
  float firstOdo = -1, lastOdo = -1, diffOdo0to5 = 0;
  float diffTime;

  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER);
  spr.setTextDatum(TL_DATUM);
  sprSetFont(fontFont2);
  sprDrawString("CONSUMPTION | DISCH.100%->4% SOC", 2, zeroY);

  spr.setTextDatum(TR_DATUM);

  sprSetFont(fontFont2);
  sprDrawString("dis./char.kWh", 128, zeroY + (1 * 15));
  sprDrawString(((liveData->settings.distanceUnit == 'k') ? "km" : "mi"), 160, zeroY + (1 * 15));
  sprDrawString("kWh100", 224, zeroY + (1 * 15));
  sprDrawString("avg.speed", 310, zeroY + (1 * 15));

  for (int i = 0; i <= 10; i++)
  {
    sprintf(tmpStr1, "%d%%", (i == 0) ? 5 : i * 10);
    sprDrawString(tmpStr1, 32, zeroY + ((12 - i) * 15));

    firstCed = (liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : firstCed;
    lastCed = (lastCed == -1 && liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : lastCed;
    firstCec = (liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : firstCec;
    lastCec = (lastCec == -1 && liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : lastCec;
    firstOdo = (liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : firstOdo;
    lastOdo = (lastOdo == -1 && liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : lastOdo;

    if (i != 10)
    {
      diffCec = (liveData->params.soc10cec[i + 1] != -1 && liveData->params.soc10cec[i] != -1) ? (liveData->params.soc10cec[i] - liveData->params.soc10cec[i + 1]) : 0;
      diffCed = (liveData->params.soc10ced[i + 1] != -1 && liveData->params.soc10ced[i] != -1) ? (liveData->params.soc10ced[i + 1] - liveData->params.soc10ced[i]) : 0;
      diffOdo = (liveData->params.soc10odo[i + 1] != -1 && liveData->params.soc10odo[i] != -1) ? (liveData->params.soc10odo[i] - liveData->params.soc10odo[i + 1]) : -1;
      diffTime = (liveData->params.soc10time[i + 1] != -1 && liveData->params.soc10time[i] != -1) ? (liveData->params.soc10time[i] - liveData->params.soc10time[i + 1]) : -1;
      if (diffCec != 0)
      {
        sprintf(tmpStr1, "+%01.01f", diffCec);
        sprDrawString(tmpStr1, 128, zeroY + ((12 - i) * 15));
        diffCec0to5 = (i == 0) ? diffCec : diffCec0to5;
      }
      if (diffCed != 0)
      {
        sprintf(tmpStr1, "%01.01f", diffCed);
        sprDrawString(tmpStr1, 80, zeroY + ((12 - i) * 15));
        diffCed0to5 = (i == 0) ? diffCed : diffCed0to5;
      }
      if (diffOdo != -1)
      {
        sprintf(tmpStr1, "%01.00f", liveData->km2distance(diffOdo));
        sprDrawString(tmpStr1, 160, zeroY + ((12 - i) * 15));
        diffOdo0to5 = (i == 0) ? diffOdo : diffOdo0to5;
        if (diffTime > 0)
        {
          sprintf(tmpStr1, "%01.01f", liveData->km2distance(diffOdo) / (diffTime / 3600));
          sprDrawString(tmpStr1, 310, zeroY + ((12 - i) * 15));
        }
      }
      if (diffOdo > 0 && diffCed != 0)
      {
        sprintf(tmpStr1, "%01.1f", (-diffCed * 100.0 / liveData->km2distance(diffOdo)));
        sprDrawString(tmpStr1, 224, zeroY + ((12 - i) * 15));
      }
    }

    if (diffOdo == -1 && liveData->params.soc10odo[i] != -1)
    {
      sprintf(tmpStr1, "%01.00f", liveData->km2distance(liveData->params.soc10odo[i]));
      sprDrawString(tmpStr1, 160, zeroY + ((12 - i) * 15));
    }
  }

  sprDrawString("0%", 32, zeroY + (13 * 15));
  sprDrawString("0-5% is calculated (same) as 5-10%", 310, zeroY + (13 * 15));

  sprDrawString("TOT.", 32, zeroY + (14 * 15));
  diffCed = (lastCed != -1 && firstCed != -1) ? firstCed - lastCed + diffCed0to5 : 0;
  sprintf(tmpStr1, "%01.01f", diffCed);
  sprDrawString(tmpStr1, 80, zeroY + (14 * 15));
  diffCec = (lastCec != -1 && firstCec != -1) ? lastCec - firstCec + diffCec0to5 : 0;
  sprintf(tmpStr1, "+%01.01f", diffCec);
  sprDrawString(tmpStr1, 128, zeroY + (14 * 15));
  diffOdo = (lastOdo != -1 && firstOdo != -1) ? lastOdo - firstOdo + diffOdo0to5 : 0;
  sprintf(tmpStr1, "%01.00f", liveData->km2distance(diffOdo));
  sprDrawString(tmpStr1, 160, zeroY + (14 * 15));
  sprintf(tmpStr1, "AVAIL.CAP: %01.01f kWh", -diffCed - diffCec);
  sprDrawString(tmpStr1, 310, zeroY + (14 * 15));
}

/**
 * Draws debug information to the screen. (screen 6)
 *
 * Displays diagnostic values for debugging purposes:
 * - App version
 * - Settings version
 * - Frames per second
 * - Wifi status
 * - Remote upload module status
 * - SD card status
 * - Voltmeter status
 * - Sleep mode
 * - GPS status
 * - Current time
 */
void Board320_240::drawSceneDebug()
{
  String tmpStr;

  spr.setTextFont(2);
  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER);
  spr.setTextDatum(TL_DATUM);
  sprSetFont(fontFont2);
  spr.setCursor(0, 0);

  /* Spotzify [SE]: Diagnostic values I would love to have :
FPS, or more specific time for one total loop of program.
COMMU stats
GPS stats
Time and date
GSM status
ABRP status
SD status*/

  // BASIC INFO
  spr.print("APP ");
  spr.print(APP_VERSION);
  spr.print(" | Settings v");
  spr.print(liveData->settings.settingsVersion);
  spr.print(" | FPS ");
  spr.println(displayFps);

  // TODO Cartype liveData->settings.carType - translate car number to string
  // TODO Adapter type liveData->settings.commType == COMM_TYPE_OBD2BLE4
  // TODO data from adapter

  // WIFI
  spr.print("WIFI ");
  spr.print(liveData->settings.wifiEnabled == 1 ? "ON" : "OFF");
  // if (liveData->params.isWifiBackupLive == true)
  spr.print(" IP ");
  spr.print(WiFi.localIP().toString());
  spr.print(" SSID ");
  spr.println(liveData->settings.wifiSsid);

  // REMOTE UPLOAD
  spr.print("REMOTE UPLOAD ");
  switch (liveData->settings.remoteUploadModuleType)
  {
  case REMOTE_UPLOAD_OFF:
    spr.print("OFF");
    break;
  case REMOTE_UPLOAD_WIFI:
    spr.print("WIFI");
    break;
  default:
    spr.print("unknown");
  }

  spr.println("");
  spr.print("CarMode: ");
  spr.println(liveData->params.carMode);

  spr.print("VIN: ");
  spr.println(liveData->params.carVin);

  spr.print("Power (kW): ");
  spr.println(liveData->params.batPowerKw * -1);

  spr.print("ignitionOn: ");
  spr.println(liveData->params.ignitionOn == 1 ? "ON" : "OFF");

  spr.print("chargingOn: ");
  spr.println(liveData->params.chargingOn == 1 ? "ON" : "OFF");

  spr.print("AC charger connected: ");
  spr.println(liveData->params.chargerACconnected == 1 ? "ON" : "OFF");

  spr.print("DC charger connected: ");
  spr.println(liveData->params.chargerDCconnected == 1 ? "ON" : "OFF");

  spr.print("Forward drive mode: : ");
  spr.println(liveData->params.forwardDriveMode == 1 ? "ON" : "OFF");

  spr.print("Reverse drive mode: : ");
  spr.println(liveData->params.reverseDriveMode == 1 ? "ON" : "OFF");

  /*
    jsonData["power"] = liveData->params.batPowerKw * -1;
      jsonData["is_parked"] = (liveData->params.parkModeOrNeutral) ? 1 : 0;

    bool ignitionOn;
    bool chargingOn;
    bool chargerACconnected;
    bool chargerDCconnected;
    bool forwardDriveMode;
    bool reverseDriveMode;
    bool parkModeOrNeutral;
    */

  // TODO sent status, ms from last sent
  // spr.println("");

  // SDCARD
  spr.print("SDCARD ");
  spr.print((liveData->settings.sdcardEnabled == 0) ? "OFF" : (strlen(liveData->params.sdcardFilename) != 0) ? "WRITE"
                                                          : (liveData->params.sdcardInit)                    ? "READY"
                                                                                                             : "MOUNTED");
  spr.print(" used ");
  spr.print(SD.usedBytes() / 1048576);
  spr.print("/");
  spr.print(SD.totalBytes() / 1048576);
  spr.println("MB");

  // VOLTMETER INA3221
  spr.print("VOLTMETER ");
  spr.println(liveData->settings.voltmeterEnabled == 1 ? "ON" : "OFF");
  if (liveData->settings.voltmeterEnabled == 1)
  {
    syslog->print("ch1:");
    syslog->print(ina3221.getBusVoltage_V(1));
    syslog->print("V\t ch2:");
    syslog->print(ina3221.getBusVoltage_V(2));
    syslog->print("V\t ch3:");
    syslog->print(ina3221.getBusVoltage_V(3));
    syslog->println("V");
    syslog->print("ch1:");
    syslog->print(ina3221.getCurrent_mA(1));
    syslog->print("mA\t ch2:");
    syslog->print(ina3221.getCurrent_mA(2));
    syslog->print("mA\t ch3:");
    syslog->print(ina3221.getCurrent_mA(3));
    syslog->println("mA");
  }

  // SLEEP MODE
  spr.print("SLEEP MODE ");
  switch (liveData->settings.sleepModeLevel)
  {
  case SLEEP_MODE_OFF:
    spr.print("OFF");
    break;
  case SLEEP_MODE_SCREEN_ONLY:
    spr.print("SCREEN ONLY");
    break;
  default:
    spr.print("UNKNOWN");
  }
  spr.println("");

  // GPS
  spr.print("GPS ");
  spr.print(liveData->params.gpsValid ? "OK" : "-");
  if (liveData->params.gpsValid)
  {
    spr.print(" ");
    spr.print(liveData->params.gpsLat);
    spr.print("/");
    spr.print(liveData->params.gpsLon);
    spr.print(" alt ");
    spr.print(liveData->params.gpsAlt);
    spr.print("m sat ");
    spr.print(liveData->params.gpsSat);
  }
  spr.println("");

  // CURRENT TIME
  spr.print("TIME ");
  spr.print(liveData->params.ntpTimeSet == 1 ? " NTP " : "");
  spr.print(liveData->params.currTimeSyncWithGps == 1 ? "GPS " : "");
  struct tm now = cachedNow;
  if (cachedNowEpoch == 0)
  {
    sprintf(tmpStr1, "-- -- -- --:--:--");
  }
  else
  {
    sprintf(tmpStr1, "%02d-%02d-%02d %02d:%02d:%02d", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
  }
  spr.println(tmpStr1);
}
