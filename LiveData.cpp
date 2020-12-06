
#ifndef LIVEDATA_CPP
#define LIVEDATA_CPP

#include "LiveData.h"
#include "menu.h"

/**
   Init params with default values
*/
void LiveData::initParams() {

  params.automaticShutdownTimer = 0;
  // SIM
  params.lastDataSent = 0;
  params.sim800l_enabled = false;
  // SD card
  params.sdcardInit = false;
  params.sdcardRecording = false;
  String tmpStr = "";
  tmpStr.toCharArray(params.sdcardFilename, tmpStr.length() + 1);
  params.sdcardCanNotify = false;
  // Car data
  params.ignitionOn = false;
  params.ignitionOnPrevious = false;
  params.operationTimeSec = 0;
  params.chargingStartTime = params.currentTime = 0;
  params.lightInfo = 0;
  params.headLights = false;
  params.dayLights = false;
  params.brakeLights = false;
  params.brakeLightInfo = 0;
  params.forwardDriveMode = false;
  params.reverseDriveMode = false;
  params.parkModeOrNeutral = false;
  params.espState = 0;
  params.speedKmh = -1;
  params.motorRpm = -1;
  params.odoKm = -1;
  params.socPerc = -1;
  params.socPercPrevious = -1;
  params.sohPerc = -1;
  params.cumulativeEnergyChargedKWh = -1;
  params.cumulativeEnergyChargedKWhStart = -1;
  params.cumulativeEnergyDischargedKWh = -1;
  params.cumulativeEnergyDischargedKWhStart = -1;
  params.availableChargePower = -1;
  params.availableDischargePower = -1;
  params.isolationResistanceKOhm = -1;
  params.batPowerAmp = -1;
  params.batPowerKw = -1;
  params.batPowerKwh100 = -1;
  params.batVoltage = -1;
  params.batCellMinV = -1;
  params.batCellMaxV = -1;
  params.batTempC = -1;
  params.batHeaterC = -1;
  params.batInletC = -1;
  params.batFanStatus = -1;
  params.batFanFeedbackHz = -1;
  params.batMinC = -1;
  params.batMaxC = -1;
  for (int i = 0; i < 12; i++) {
    params.batModuleTempC[i] = 0;
  }
  params.batModuleTempC[0] = -1;
  params.batModuleTempC[1] = -1;
  params.batModuleTempC[2] = -1;
  params.batModuleTempC[3] = -1;
  params.coolingWaterTempC = -1;
  params.coolantTemp1C = -1;
  params.coolantTemp2C = -1;
  params.bmsUnknownTempA = -1;
  params.bmsUnknownTempB = -1;
  params.bmsUnknownTempC = -1;
  params.bmsUnknownTempD = -1;
  params.auxPerc = -1;
  params.auxCurrentAmp = -1;
  params.auxVoltage = -1;
  params.indoorTemperature = -1;
  params.outdoorTemperature = -1;
  params.tireFrontLeftTempC = -1;
  params.tireFrontLeftPressureBar = -1;
  params.tireFrontRightTempC = -1;
  params.tireFrontRightPressureBar = -1;
  params.tireRearLeftTempC = -1;
  params.tireRearLeftPressureBar = -1;
  params.tireRearRightTempC = -1;
  params.tireRearRightPressureBar = -1;
  for (int i = 0; i <= 10; i++) {
    params.soc10ced[i] = params.soc10cec[i] = params.soc10odo[i] = -1;
    params.soc10time[i] = 0;
  }
  for (int i = 0; i < 98; i++) {
    params.cellVoltage[i] = 0;
  }
  params.cellCount = 0;
  for (int i = 0; i <= 100; i++) {
    params.chargingGraphMinKw[i] = -1;
    params.chargingGraphMaxKw[i] = -1;
    params.chargingGraphBatMinTempC[i] = -100;
    params.chargingGraphBatMaxTempC[i] = -100;
    params.chargingGraphHeaterTempC[i] = -100;
    params.chargingGraphWaterCoolantTempC[i] = -100;
  }

  // Menu
  menuItemsCount = sizeof(menuItemsSource) / sizeof(menuItemsSource[0]);
  menuItems = menuItemsSource;
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
  return (settings.distanceUnit == 'k') ? inKm : inKm / 1.609344;
}

/**
   Convert celsius to celsius or farenheit
*/
float LiveData::celsius2temperature(float inCelsius) {
  return (settings.temperatureUnit == 'c') ? inCelsius : (inCelsius * 1.8) + 32;
}

/**
   Convert bar to bar or psi
*/
float LiveData::bar2pressure(float inBar) {
  return (settings.pressureUnit == 'b') ? inBar : inBar * 14.503773800722;
}




//
#endif // LIVEDATA_CPP
