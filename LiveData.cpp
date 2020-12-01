
#ifndef LIVEDATA_CPP
#define LIVEDATA_CPP

#include "LiveData.h"
#include "menu.h"

/**
   Init params with default values
*/
void LiveData::initParams() {

  this->params.automaticShutdownTimer = 0;
#ifdef SIM800L_ENABLED
  this->params.lastDataSent = 0;
  this->params.sim800l_enabled = false;
#endif //SIM800L_ENABLED
  this->params.ignitionOn = false;
  this->params.ignitionOnPrevious = false;
  this->params.chargingStartTime = this->params.currentTime = 0;
  this->params.lightInfo = 0;
  this->params.headLights = false;
  this->params.dayLights = false;
  this->params.brakeLights = false;
  this->params.brakeLightInfo = 0;
  this->params.forwardDriveMode = false;
  this->params.reverseDriveMode = false;
  this->params.parkModeOrNeutral = false;
  this->params.espState = 0;
  this->params.speedKmh = -1;
  this->params.motorRpm = -1;
  this->params.odoKm = -1;
  this->params.socPerc = -1;
  this->params.socPercPrevious = -1;
  this->params.sohPerc = -1;
  this->params.cumulativeEnergyChargedKWh = -1;
  this->params.cumulativeEnergyChargedKWhStart = -1;
  this->params.cumulativeEnergyDischargedKWh = -1;
  this->params.cumulativeEnergyDischargedKWhStart = -1;
  this->params.availableChargePower = -1;
  this->params.availableDischargePower = -1;
  this->params.isolationResistanceKOhm = -1;
  this->params.batPowerAmp = -1;
  this->params.batPowerKw = -1;
  this->params.batPowerKwh100 = -1;
  this->params.batVoltage = -1;
  this->params.batCellMinV = -1;
  this->params.batCellMaxV = -1;
  this->params.batTempC = -1;
  this->params.batHeaterC = -1;
  this->params.batInletC = -1;
  this->params.batFanStatus = -1;
  this->params.batFanFeedbackHz = -1;
  this->params.batMinC = -1;
  this->params.batMaxC = -1;
  for (int i = 0; i < 12; i++) {
    this->params.batModuleTempC[i] = 0;
  }
  this->params.batModuleTempC[0] = -1;
  this->params.batModuleTempC[1] = -1;
  this->params.batModuleTempC[2] = -1;
  this->params.batModuleTempC[3] = -1;
  this->params.coolingWaterTempC = -1;
  this->params.coolantTemp1C = -1;
  this->params.coolantTemp2C = -1;
  this->params.bmsUnknownTempA = -1;
  this->params.bmsUnknownTempB = -1;
  this->params.bmsUnknownTempC = -1;
  this->params.bmsUnknownTempD = -1;
  this->params.auxPerc = -1;
  this->params.auxCurrentAmp = -1;
  this->params.auxVoltage = -1;
  this->params.indoorTemperature = -1;
  this->params.outdoorTemperature = -1;
  this->params.tireFrontLeftTempC = -1;
  this->params.tireFrontLeftPressureBar = -1;
  this->params.tireFrontRightTempC = -1;
  this->params.tireFrontRightPressureBar = -1;
  this->params.tireRearLeftTempC = -1;
  this->params.tireRearLeftPressureBar = -1;
  this->params.tireRearRightTempC = -1;
  this->params.tireRearRightPressureBar = -1;
  for (int i = 0; i <= 10; i++) {
    this->params.soc10ced[i] = this->params.soc10cec[i] = this->params.soc10odo[i] = -1;
    this->params.soc10time[i] = 0;
  }
  for (int i = 0; i < 98; i++) {
    this->params.cellVoltage[i] = 0;
  }
  this->params.cellCount = 0;
  for (int i = 0; i <= 100; i++) {
    this->params.chargingGraphMinKw[i] = -1;
    this->params.chargingGraphMaxKw[i] = -1;
    this->params.chargingGraphBatMinTempC[i] = -100;
    this->params.chargingGraphBatMaxTempC[i] = -100;
    this->params.chargingGraphHeaterTempC[i] = -100;
    this->params.chargingGraphWaterCoolantTempC[i] = -100;
  }

  // Menu
  this->menuItems = menuItemsSource;
}

/**
  Hex to dec (1-2 byte values, signed/unsigned)
  For 4 byte change int to long and add part for signed numbers
*/
float LiveData::hexToDec(String hexString, byte bytes, bool signedNum) {

  unsigned int decValue = 0;
  unsigned int nextInt;

  for (int i = 0; i < hexString.length(); i++) {
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
  }

  // Unsigned - do nothing
  if (!signedNum) {
    return decValue;
  }
  // Signed for 1, 2 bytes
  if (bytes == 1) {
    return (decValue > 127 ? (float)decValue - 256.0 : decValue);
  }
  return (decValue > 32767 ? (float)decValue - 65536.0 : decValue);
}

/**
   Convert km to km or miles
*/
float LiveData::km2distance(float inKm) {
  return (this->settings.distanceUnit == 'k') ? inKm : inKm / 1.609344;
}

/**
   Convert celsius to celsius or farenheit
*/
float LiveData::celsius2temperature(float inCelsius) {
  return (this->settings.temperatureUnit == 'c') ? inCelsius : (inCelsius * 1.8) + 32;
}

/**
   Convert bar to bar or psi
*/
float LiveData::bar2pressure(float inBar) {
  return (this->settings.pressureUnit == 'b') ? inBar : inBar * 14.503773800722;
}




//
#endif // LIVEDATA_CPP
