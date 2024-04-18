/**
 * Initializes LiveData parameters with default values.
 * Called on startup to initialize the LiveData params struct.
 * Sets all params to default values.
 */
#include "LiveData.h"
#include "menu.h"

LogSerial *syslog;

/**
   Debug level
*/
void debug(String msg, uint8_t debugLevel)
{
  syslog->println(msg);
}

/**
   Init params with default values
*/
void LiveData::initParams()
{

  params.queueLoopCounter = 0;
  params.stopCommandQueue = false;
  // Network
  params.ntpTimeSet = false;
  params.lastDataSent = 0;
  params.lastContributeSent = 0;
  params.sim800l_enabled = false; // UNSUPPORTED MODULE
  params.sim800l_lastOkReceiveTime = 0; // UNSUPPORTED MODULE
  params.lastSuccessNetSendTime = 0;
  params.isWifiBackupLive = false;
  params.wifiLastConnectedTime = 0;
  params.wifiBackupUptime = 0;
  params.wifiApMode = false;
  // SD card
  params.sdcardInit = false;
  params.sdcardRecording = false;
  String tmpStr = "";
  tmpStr.toCharArray(params.sdcardFilename, tmpStr.length() + 1);
  tmpStr.toCharArray(params.sdcardAbrpFilename, tmpStr.length() + 1);
  params.sdcardCanNotify = false;
  // Gps
  params.currTimeSyncWithGps = false;
  params.gpsValid = false;
  params.gpsLat = -1.0;
  params.gpsLon = -1.0;
  params.gpsSat = 0;
  params.gpsAlt = -500;
  params.setGpsTimeFromCar = 0;
  params.gyroSensorMotion = false;
  // Display
  params.displayScreen = SCREEN_SPEED;
  params.displayScreenAutoMode = SCREEN_AUTO;
  params.lastButtonPushedTime = 0;
  params.lcdBrightnessCalc = -1; // calculated for automatic mode
  params.spriteInit = false;
  // VoltageMeter INA3221
  params.lastVoltageReadTime = 0;
  params.lastVoltageOkTime = 0;
  params.stopCommandQueueTime = 0;
  // Car data
  params.carMode = CAR_MODE_NONE;
  params.sleepModeQueue = false;
  params.getValidResponse = false;
  params.wakeUpTime = 0;
  params.ignitionOn = false;
  params.lastIgnitionOnTime = 0;
  params.operationTimeSec = 0;
  params.chargingStartTime = params.currentTime = 0;
  params.chargingOn = false;
  params.chargerACconnected = false;
  params.chargerDCconnected = false;
  params.headLights = false;
  params.autoLights = false;
  params.dayLights = false;
  params.brakeLights = false;
  params.trunkDoorOpen = false;
  params.leftFrontDoorOpen = false;
  params.rightFrontDoorOpen = false;
  params.leftRearDoorOpen = false;
  params.rightRearDoorOpen = false;
  params.hoodDoorOpen = false;
  params.timeInForwardDriveMode = 0;
  params.forwardDriveMode = false;
  params.reverseDriveMode = false;
  params.parkModeOrNeutral = false;
  params.speedKmh = -1;
  params.avgSpeedKmh = 0;
  params.speedKmhGPS = -1;
  params.motor1Rpm = params.motor2Rpm = -1;
  params.inverterTempC = -100;
  params.motorTempC = -100;
  params.odoKm = -1;
  params.odoKmStart = -1;
  params.socPercBms = -1;
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
  params.batEnergyContent = -1;
  params.batMaxEnergyContent = -1;
  params.batPowerAmp = -1000;
  params.batPowerKw = -1000;
  params.batPowerKwh100 = -1;
  params.batVoltage = -1;
  params.batCellMinV = -1;
  params.batCellMaxV = -1;
  params.batCellMinVNo = 255;
  params.batCellMaxVNo = 255;
  params.batTempC = -100;
  params.batHeaterC = -100;
  params.batInletC = -100;
  params.batFanStatus = -1;
  params.batFanFeedbackHz = -1;
  params.batMinC = -100;
  params.batMaxC = -100;
  for (int i = 0; i < 25; i++)
  {
    params.batModuleTempC[i] = -100;
  }
  params.batModuleTempC[0] = -100;
  params.batModuleTempC[1] = -100;
  params.batModuleTempC[2] = -100;
  params.batModuleTempC[3] = -100;
  params.coolingWaterTempC = -100;
  params.coolantTemp1C = -100;
  params.coolantTemp2C = -100;
  params.bmsUnknownTempA = -100;
  params.bmsUnknownTempB = -100;
  params.bmsUnknownTempC = -100;
  params.bmsUnknownTempD = -100;
  params.batteryManagementMode = BAT_MAN_MODE_NOT_IMPLEMENTED;
  params.auxPerc = -1;
  params.auxCurrentAmp = -1000;
  params.auxVoltage = -1;
  params.indoorTemperature = -100;
  params.outdoorTemperature = -100;
  params.evaporatorTempC = -100;
  params.tireFrontLeftTempC = -100;
  params.tireFrontLeftPressureBar = -1;
  params.tireFrontRightTempC = -100;
  params.tireFrontRightPressureBar = -1;
  params.tireRearLeftTempC = -100;
  params.tireRearLeftPressureBar = -1;
  params.tireRearRightTempC = -100;
  params.tireRearRightPressureBar = -1;
  for (int i = 0; i <= 10; i++)
  {
    params.soc10ced[i] = params.soc10cec[i] = params.soc10odo[i] = -1;
    params.soc10time[i] = 0;
  }
  for (int i = 0; i < 200; i++)
  { // id3 58kWh = 108 cells
    params.cellVoltage[i] = -1;
  }
  params.cellCount = 0;
  for (int i = 0; i <= 100; i++)
  {
    params.chargingGraphMinKw[i] = -1;
    params.chargingGraphMaxKw[i] = -1;
    params.chargingGraphBatMinTempC[i] = -100;
    params.chargingGraphBatMaxTempC[i] = -100;
    params.chargingGraphHeaterTempC[i] = -100;
    params.chargingGraphWaterCoolantTempC[i] = -100;
  }
  params.contributeStatus = CONTRIBUTE_NONE;

  // Menu
  menuItemsCount = sizeof(menuItemsSource) / sizeof(menuItemsSource[0]);
  menuItems = menuItemsSource;
}

/**
  Hex to dec (1-2 byte values, signed/unsigned)
  For 4 byte change int to long and add part for signed numbers
*/
double LiveData::hexToDec(String hexString, uint8_t bytes, bool signedNum)
{
  uint64_t range;
  if (bytes < 1 || bytes > 4)
    return -1;

  double decValue = strtoul(hexString.c_str(), NULL, 16);
  if (signedNum)
  {
    range = ((uint64_t)1) << (bytes * 8);
    if (decValue > (range - 1) / 2)
      decValue -= range;
  }
  return decValue;
}

/**
  Parsed from merged response row:
  Hex to dec (1-2 byte values, signed/unsigned)
  For 4 byte change int to long and add part for signed numbers
*/
double LiveData::hexToDecFromResponse(uint8_t from, uint8_t to, uint8_t bytes, bool signedNum)
{
  return hexToDec(responseRowMerged.substring(from, to).c_str(), bytes, signedNum);
}

/**
  Combination of responseRowMerged.substring -> strtol -> float
*/
float LiveData::decFromResponse(uint8_t from, uint8_t to, char **str_end, int base)
{
  return float(strtol(responseRowMerged.substring(from, to).c_str(), str_end, base));
}

/**
   Convert km to km or miles
*/
float LiveData::km2distance(float inKm)
{
  return (settings.distanceUnit == 'k') ? inKm : inKm / 1.609344;
}

/**
   Convert celsius to celsius or farenheit
*/
float LiveData::celsius2temperature(float inCelsius)
{
  return (settings.temperatureUnit == 'c') ? inCelsius : (inCelsius * 1.8) + 32;
}

/**
   Convert bar to bar or psi
*/
float LiveData::bar2pressure(float inBar)
{
  return (settings.pressureUnit == 'b') ? inBar : inBar * 14.503773800722;
}

/**
   batteryManagementModeStr
*/
String LiveData::getBatteryManagementModeStr(int8_t mode)
{
  switch (mode)
  {
  case BAT_MAN_MODE_LOW_TEMPERATURE_RANGE_COOLING:
    return "LTRCOOL"; // via motor (charging 75kW / bat.25°C in the winter)
  case BAT_MAN_MODE_LOW_TEMPERATURE_RANGE:
    return "LTR"; // via motor (drive)
  case BAT_MAN_MODE_COOLING:
    return "COOL"; // chiller via A/C
  case BAT_MAN_MODE_OFF:
    return "OFF"; // water pump off
  case BAT_MAN_MODE_PTC_HEATER:
    return "PTC"; // PTC heater
  case BAT_MAN_MODE_UNKNOWN:
    return "UNK";
  default:
    return "";
  }
}

/**
 * Automatic clear of driving/charging data
 */
void LiveData::clearDrivingAndChargingStats(int newCarMode)
{
  params.carMode = newCarMode;
  params.carModeChanged = params.currentTime;

  params.cumulativeEnergyDischargedKWhStart = params.cumulativeEnergyDischargedKWh;
  params.cumulativeEnergyChargedKWhStart = params.cumulativeEnergyChargedKWh;
  params.odoKmStart = params.odoKm;

  for (int i = 0; i <= 100; i++)
  {
    params.chargingGraphMinKw[i] = -1;
    params.chargingGraphMaxKw[i] = -1;
    params.chargingGraphBatMinTempC[i] = -100;
    params.chargingGraphBatMaxTempC[i] = -100;
    params.chargingGraphHeaterTempC[i] = -100;
    params.chargingGraphWaterCoolantTempC[i] = -100;
  }
}

/**
 * Automatically turn off CAN scanning when 2 minute inactive
    - car stopped, not in D/R drive mode
    - car is not charging
    - voltage is under 14V (dcdc is not running)
    - TODO: BMS state - HV battery was disconnected
 */
void LiveData::prepareForStopCommandQueue()
{
  if (params.stopCommandQueueTime == 0)
  {
    params.stopCommandQueueTime = params.currentTime;
  }
}

/**
 * Cancel preparation, or restore from already stopped
 */
void LiveData::continueWithCommandQueue()
{
  params.stopCommandQueueTime = 0;
  if (params.stopCommandQueue)
  {
    params.stopCommandQueue = false;
    syslog->println("CAN Command queue restored...");
  }
}