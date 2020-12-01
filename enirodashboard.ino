/*
  !! working only with OBD BLE 4.0 adapters
  !! Supported adapter is  Vgate ICar Pro (must be BLE4.0 version)
  !! Not working with standard BLUETOOTH 3 adapters

Required libraries
- esp32 board support
- tft_espi
- ArduinoJson
For SIM800L m5stack
- SIM800L.h, SoftwareSerial.h
*/

////////////////////////////////////////////////////////////
// SELECT HARDWARE
////////////////////////////////////////////////////////////

//#define BOARD_TTGO_T4
#define BOARD_M5STACK_CORE

//#define SIM800L_ENABLED

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

#define APP_VERSION "v1.9.0"
#define APP_RELEASE_DATE "2020-11-30"

#include <SPI.h>
#include <TFT_eSPI.h>

#include <BLEDevice.h>
#ifdef BOARD_TTGO_T4
#include "BoardTtgoT4v13.h"
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
#include "BoardM5stackCore.h"
#endif // BOARD_M5STACK_CORE
#include "config.h"

#include <mySD.h>
//#include <SD.h>
#include <EEPROM.h>
#include <sys/time.h>
#include <analogWrite.h>
#include "LiveData.h"
#include "menu.h"
#include "CarInterface.h"
#include "CarKiaEniro.h"
#include "CarHyundaiIoniq.h"
#include "CarKiaDebugObd2.h"

#ifdef SIM800L_ENABLED
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "SIM800L.h"

SIM800L* sim800l;
#endif //SIM800L_ENABLED

// PLEASE CHANGE THIS SETTING for your BLE4
uint32_t PIN = 1234;

// TFT, SD SPI
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
//SPIClass spiSD(HSPI);

// BLUETOOTH4
static boolean bleConnect = true;
static boolean bleConnected = false;
static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristicWrite;
BLEAdvertisedDevice* foundMyBleDevice;
BLEClient* pClient;
BLEScan* pBLEScan;

// Temporary variables
char ch;
String line;
char tmpStr1[20];
char tmpStr2[20];
char tmpStr3[20];
char tmpStr4[20];

// Screens, buttons
#define displayScreenCount 7
byte displayScreen  = SCREEN_AUTO;
byte displayScreenAutoMode = 0;
byte displayScreenSpeedHud = false;
bool btnLeftPressed   = true;
bool btnMiddlePressed = true;
bool btnRightPressed  = true;
bool testDataMode = false;

// Debug screen - next command with right button
uint16_t debugCommandIndex = 0;
String debugAtshRequest = "ATSH7E4";
String debugCommandRequest = "220101";
String debugLastString = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
String debugPreviousString = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
bool debugTmpCharging = false;
String debugTmpChargingLast05 = "";
String debugTmpChargingPrevious05 = "";
String debugTmpChargingRef05 = "620105003FFF900000000000000000--####################--------00000000--0000----##00--00000000AAAA";
String debugTmpChargingLast06 = "";
String debugTmpChargingPrevious06 = "";
String debugTmpChargingRef06 = "620106FFFFFFFF--00--########################00--28EA00";

// Car
CarInterface* carInterface;
LiveData* liveData;

/**
   Clear screen a display two lines message
*/
bool displayMessage(const char* row1, const char* row2) {

  // Must draw directly, withou sprite (due to psramFound check)
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(BL_DATUM);
  tft.drawString(row1, 0, 240 / 2, GFXFF);
  tft.drawString(row2, 0, (240 / 2) + 30, GFXFF);

  return true;
}

/**
  Shutdown device
*/
bool shutdownDevice() {

  Serial.println("Shutdown.");

  displayMessage("Shutdown in 3 sec.", "");
  delay(3000);

  setCpuFrequencyMhz(80);
  analogWrite(TFT_BL, 0);
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);
  btStop();
  //adc_power_off();
  //esp_wifi_stop();
  esp_bt_controller_disable();

  delay(2000);
  //esp_sleep_enable_timer_wakeup(/*minutes*/ 525600L * 60L * 1000000L);
  esp_deep_sleep_start();

  return true;
}

/**
  Load settings from flash memory, upgrade structure if version differs
*/
bool saveSettings() {

  // Flash to memory
  Serial.println("Settings saved to eeprom.");
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();

  return true;
}

/**
  Reset settings (factory reset)
*/
bool resetSettings() {

  // Flash to memory
  Serial.println("Factory reset.");
  liveData->settings.initFlag = 1;
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();

  displayMessage("Settings erased", "Restarting in 5 seconds");

  delay(5000);
  ESP.restart();

  return true;
}

/**
  Load setting from flash memory, upgrade structure if version differs
*/
bool loadSettings() {

  String tmpStr;

  // Init
  liveData->settings.initFlag = 183;
  liveData->settings.settingsVersion = 3;
  liveData->settings.carType = CAR_KIA_ENIRO_2020_64;

  // Default OBD adapter MAC and UUID's
  tmpStr = "00:00:00:00:00:00"; // Pair via menu (middle button)
  tmpStr.toCharArray(liveData->settings.obdMacAddress, 18);
  tmpStr = "000018f0-0000-1000-8000-00805f9b34fb"; // Default UUID's for VGate iCar Pro BLE4 adapter
  tmpStr.toCharArray(liveData->settings.serviceUUID, 37);
  tmpStr = "00002af0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(liveData->settings.charTxUUID, 37);
  tmpStr = "00002af1-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(liveData->settings.charRxUUID, 37);

  liveData->settings.displayRotation = 1; // 1,3
  liveData->settings.distanceUnit = 'k';
  liveData->settings.temperatureUnit = 'c';
  liveData->settings.pressureUnit = 'b';
  liveData->settings.defaultScreen = 1;
  liveData->settings.lcdBrightness = 0;
  liveData->settings.debugScreen = 0;
  liveData->settings.predrawnChargingGraphs = 1;

  // Load settings and replace default values
  Serial.println("Reading settings from eeprom.");
  EEPROM.begin(sizeof(SETTINGS_STRUC));
  EEPROM.get(0, liveData->tmpSettings);

  // Init flash with default settings
  if (liveData->tmpSettings.initFlag != 183) {
    Serial.println("Settings not found. Initialization.");
    saveSettings();
  } else {
    Serial.print("Loaded settings ver.: ");
    Serial.println(liveData->settings.settingsVersion);

    // Upgrade structure
    if (liveData->settings.settingsVersion != liveData->tmpSettings.settingsVersion) {
      if (liveData->tmpSettings.settingsVersion == 1) {
        liveData->tmpSettings.settingsVersion = 2;
        liveData->tmpSettings.defaultScreen = liveData->settings.defaultScreen;
        liveData->tmpSettings.lcdBrightness = liveData->settings.lcdBrightness;
        liveData->tmpSettings.debugScreen = liveData->settings.debugScreen;
      }
      if (liveData->tmpSettings.settingsVersion == 2) {
        liveData->tmpSettings.settingsVersion = 3;
        liveData->tmpSettings.predrawnChargingGraphs = liveData->settings.predrawnChargingGraphs;
      }
      saveSettings();
    }

    // Save version? No need to upgrade structure
    if (liveData->settings.settingsVersion == liveData->tmpSettings.settingsVersion) {
      liveData->settings = liveData->tmpSettings;
    }
  }

  // Load command queue
  if (liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64 ||
      liveData->settings.carType == CAR_KIA_ENIRO_2020_39 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    CarKiaEniro* tmpCar = new CarKiaEniro();
    carInterface = tmpCar;
  } else
  if (liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018) {
    CarHyundaiIoniq* tmpCar = new CarHyundaiIoniq();
    carInterface = tmpCar;
  } else {
    // if (liveData->settings.carType == CAR_DEBUG_OBD2_KIA)   
    CarKiaDebugObd2* tmpCar = new CarKiaDebugObd2();
    carInterface = tmpCar;
  }
  carInterface->setLiveData(liveData);
  carInterface->activateCommandQueue();
  debugCommandIndex = liveData->commandQueueLoopFrom;

  return true;
}

/**
   Init structure with data
*/
bool initStructure() {

  liveData->params.automatickShutdownTimer = 0;
#ifdef SIM800L_ENABLED
  liveData->params.lastDataSent = 0;
  liveData->params.sim800l_enabled = true;
#endif //SIM800L_ENABLED
  liveData->params.ignitionOn = false;
  liveData->params.ignitionOnPrevious = false;
  liveData->params.chargingStartTime = liveData->params.currentTime = 0;
  liveData->params.lightInfo = 0;
  liveData->params.headLights = false;
  liveData->params.dayLights = false;
  liveData->params.brakeLights = false;
  liveData->params.brakeLightInfo = 0;
  liveData->params.forwardDriveMode = false;
  liveData->params.reverseDriveMode = false;
  liveData->params.parkModeOrNeutral = false;
  liveData->params.espState = 0;
  liveData->params.speedKmh = -1;
  liveData->params.motorRpm = -1;
  liveData->params.odoKm = -1;
  liveData->params.socPerc = -1;
  liveData->params.socPercPrevious = -1;
  liveData->params.sohPerc = -1;
  liveData->params.cumulativeEnergyChargedKWh = -1;
  liveData->params.cumulativeEnergyChargedKWhStart = -1;
  liveData->params.cumulativeEnergyDischargedKWh = -1;
  liveData->params.cumulativeEnergyDischargedKWhStart = -1;
  liveData->params.availableChargePower = -1;
  liveData->params.availableDischargePower = -1;
  liveData->params.isolationResistanceKOhm = -1;
  liveData->params.batPowerAmp = -1;
  liveData->params.batPowerKw = -1;
  liveData->params.batPowerKwh100 = -1;
  liveData->params.batVoltage = -1;
  liveData->params.batCellMinV = -1;
  liveData->params.batCellMaxV = -1;
  liveData->params.batTempC = -1;
  liveData->params.batHeaterC = -1;
  liveData->params.batInletC = -1;
  liveData->params.batFanStatus = -1;
  liveData->params.batFanFeedbackHz = -1;
  liveData->params.batMinC = -1;
  liveData->params.batMaxC = -1;
  for (int i = 0; i < 12; i++) {
    liveData->params.batModuleTempC[i] = 0;
  }
  liveData->params.batModuleTempC[0] = -1;
  liveData->params.batModuleTempC[1] = -1;
  liveData->params.batModuleTempC[2] = -1;
  liveData->params.batModuleTempC[3] = -1;
  liveData->params.coolingWaterTempC = -1;
  liveData->params.coolantTemp1C = -1;
  liveData->params.coolantTemp2C = -1;
  liveData->params.bmsUnknownTempA = -1;
  liveData->params.bmsUnknownTempB = -1;
  liveData->params.bmsUnknownTempC = -1;
  liveData->params.bmsUnknownTempD = -1;
  liveData->params.auxPerc = -1;
  liveData->params.auxCurrentAmp = -1;
  liveData->params.auxVoltage = -1;
  liveData->params.indoorTemperature = -1;
  liveData->params.outdoorTemperature = -1;
  liveData->params.tireFrontLeftTempC = -1;
  liveData->params.tireFrontLeftPressureBar = -1;
  liveData->params.tireFrontRightTempC = -1;
  liveData->params.tireFrontRightPressureBar = -1;
  liveData->params.tireRearLeftTempC = -1;
  liveData->params.tireRearLeftPressureBar = -1;
  liveData->params.tireRearRightTempC = -1;
  liveData->params.tireRearRightPressureBar = -1;
  for (int i = 0; i <= 10; i++) {
    liveData->params.soc10ced[i] = liveData->params.soc10cec[i] = liveData->params.soc10odo[i] = -1;
    liveData->params.soc10time[i] = 0;
  }
  for (int i = 0; i < 98; i++) {
    liveData->params.cellVoltage[i] = 0;
  }
  liveData->params.cellCount = 0;
  for (int i = 0; i <= 100; i++) {
    liveData->params.chargingGraphMinKw[i] = -1;
    liveData->params.chargingGraphMaxKw[i] = -1;
    liveData->params.chargingGraphBatMinTempC[i] = -100;
    liveData->params.chargingGraphBatMaxTempC[i] = -100;
    liveData->params.chargingGraphHeaterTempC[i] = -100;
    liveData->params.chargingGraphWaterCoolantTempC[i] = -100;
  }

  return true;
}

/**
   Convert km to km or miles
*/
float km2distance(float inKm) {
  return (liveData->settings.distanceUnit == 'k') ? inKm : inKm / 1.609344;
}

/**
   Convert celsius to celsius or farenheit
*/
float celsius2temperature(float inCelsius) {
  return (liveData->settings.temperatureUnit == 'c') ? inCelsius : (inCelsius * 1.8) + 32;
}

/**
   Convert bar to bar or psi
*/
float bar2pressure(float inBar) {
  return (liveData->settings.pressureUnit == 'b') ? inBar : inBar * 14.503773800722;
}

/**
  Draw cell on dashboard
*/
bool monitoringRect(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, uint16_t bgColor, uint16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextColor(TFT_SILVER, bgColor); // Bk, fg color
  spr.setTextSize(1); // Size for small 5x7 font
  spr.drawString(desc, posx, posy, 2);

  // Big 2x2 cell in the middle of screen
  if (w == 2 && h == 2) {

    // Bottom 2 numbers with charged/discharged kWh from start
    posx = (x * 80) + 5;
    posy = ((y + h) * 60) - 32;
    sprintf(tmpStr3, "-%01.01f", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
    spr.setFreeFont(&Roboto_Thin_24);
    spr.setTextDatum(TL_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);

    posx = ((x + w) * 80) - 8;
    sprintf(tmpStr3, "+%01.01f", liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
    spr.setTextDatum(TR_DATUM);
    spr.drawString(tmpStr3, posx, posy, GFXFF);

    // Main number - kwh on roads, amps on charges
    posy = (y * 60) + 24;
    spr.setTextColor(fgColor, bgColor);
    spr.setFreeFont(&Orbitron_Light_32);
    spr.drawString(text, posx, posy, 7);

  } else {

    // All others 1x1 cells
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(fgColor, bgColor);
    spr.setFreeFont(&Orbitron_Light_24);
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    spr.drawString(text, posx, posy, (w == 2 ? 7 : GFXFF));
  }

  return true;
}

/**
  Draw small rect 80x30
*/
bool drawSmallRect(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, int16_t bgColor, int16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 32) + 1;

  spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 32) - 1, h * 32, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 32) - 1, w * 80, TFT_BLACK);
  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextColor(TFT_SILVER, bgColor); // Bk, fg bgColor
  spr.setTextSize(1); // Size for small 5x7 font
  spr.drawString(desc, posx, posy, 2);

  spr.setTextDatum(TC_DATUM);
  spr.setTextColor(fgColor, bgColor);
  posx = (x * 80) + (w * 80 / 2) - 3;
  spr.drawString(text, posx, posy + 14, 2);

  return true;
}

/**
  Show tire pressures / temperatures
  Custom field
*/
bool showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char* topleft, const char* topright, const char* bottomleft, const char* bottomright, uint16_t color) {

  int32_t posx, posy;

  spr.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  color);
  spr.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  spr.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);

  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER, color);
  spr.setTextSize(1);
  posx = (x * 80) + 4;
  posy = (y * 60) + 0;
  spr.drawString(topleft, posx, posy, 2);
  posy = (y * 60) + 14;
  spr.drawString(bottomleft, posx, posy, 2);

  spr.setTextDatum(TR_DATUM);
  posx = ((x + w) * 80) - 4;
  posy = (y * 60) + 0;
  spr.drawString(topright, posx, posy, 2);
  posy = (y * 60) + 14;
  spr.drawString(bottomright, posx, posy, 2);

  return true;
}

/**
   Main screen (Screen 1)
*/
bool drawSceneMain() {

  // Tire pressure
  char pressureStr[4] = "bar";
  char temperatureStr[2] = "C";
  if (liveData->settings.pressureUnit != 'b')
    strcpy(pressureStr, "psi");
  if (liveData->settings.temperatureUnit != 'c')
    strcpy(temperatureStr, "F");
  sprintf(tmpStr1, "%01.01f%s %02.00f%s", bar2pressure(liveData->params.tireFrontLeftPressureBar), pressureStr, celsius2temperature(liveData->params.tireFrontLeftTempC), temperatureStr);
  sprintf(tmpStr2, "%02.00f%s %01.01f%s", celsius2temperature(liveData->params.tireFrontRightTempC), temperatureStr, bar2pressure(liveData->params.tireFrontRightPressureBar), pressureStr);
  sprintf(tmpStr3, "%01.01f%s %02.00f%s", bar2pressure(liveData->params.tireRearLeftPressureBar), pressureStr, celsius2temperature(liveData->params.tireRearLeftTempC), temperatureStr);
  sprintf(tmpStr4, "%02.00f%s %01.01f%s", celsius2temperature(liveData->params.tireRearRightTempC), temperatureStr, bar2pressure(liveData->params.tireRearRightPressureBar), pressureStr);
  showTires(1, 0, 2, 1, tmpStr1, tmpStr2, tmpStr3, tmpStr4, TFT_BLACK);

  // Added later - kwh total in tires box
  // TODO: refactoring
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_GREEN, TFT_BLACK);
  sprintf(tmpStr1, "C: %01.01f +%01.01fkWh", liveData->params.cumulativeEnergyChargedKWh, liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 30, 2);
  spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  sprintf(tmpStr1, "D: %01.01f -%01.01fkWh", liveData->params.cumulativeEnergyDischargedKWh, liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
  spr.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 44, 2);

  // batPowerKwh100 on roads, else batPowerAmp
  if (liveData->params.speedKmh > 20) {
    sprintf(tmpStr1, "%01.01f", km2distance(liveData->params.batPowerKwh100));
    monitoringRect(1, 1, 2, 2, tmpStr1, ((liveData->settings.distanceUnit == 'k') ? "POWER KWH/100KM" : "POWER KWH/100MI"), (liveData->params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  } else {
    // batPowerAmp on chargers (under 10kmh)
    sprintf(tmpStr1, "%01.01f", liveData->params.batPowerKw);
    monitoringRect(1, 1, 2, 2, tmpStr1, "POWER KW", (liveData->params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (liveData->params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
  }

  // socPerc
  sprintf(tmpStr1, "%01.00f%%", liveData->params.socPerc);
  sprintf(tmpStr2, (liveData->params.sohPerc ==  100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), liveData->params.sohPerc);
  monitoringRect(0, 0, 1, 1, tmpStr1, tmpStr2, (liveData->params.socPerc < 10 || liveData->params.sohPerc < 100 ? TFT_RED : (liveData->params.socPerc  > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)), TFT_WHITE);

  // batPowerAmp
  sprintf(tmpStr1, (abs(liveData->params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.batPowerAmp);
  monitoringRect(0, 1, 1, 1, tmpStr1, "CURRENT A", (liveData->params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // batVoltage
  sprintf(tmpStr1, "%03.00f", liveData->params.batVoltage);
  monitoringRect(0, 2, 1, 1, tmpStr1, "VOLTAGE", TFT_DEFAULT_BK, TFT_WHITE);

  // batCellMinV
  sprintf(tmpStr1, "%01.02f", liveData->params.batCellMaxV - liveData->params.batCellMinV);
  sprintf(tmpStr2, "CELLS %01.02f", liveData->params.batCellMinV);
  monitoringRect(0, 3, 1, 1, ( liveData->params.batCellMaxV - liveData->params.batCellMinV == 0.00 ? "OK" : tmpStr1), tmpStr2, TFT_DEFAULT_BK, TFT_WHITE);

  // batTempC
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), celsius2temperature(liveData->params.batMinC));
  sprintf(tmpStr2, ((liveData->settings.temperatureUnit == 'c') ? "BATT. %01.00fC" : "BATT. %01.01fF"), celsius2temperature(liveData->params.batMaxC));
  monitoringRect(1, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);

  // batHeaterC
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), celsius2temperature(liveData->params.batHeaterC));
  monitoringRect(2, 3, 1, 1, tmpStr1, "BAT.HEAT", TFT_TEMP, TFT_WHITE);

  // Aux perc
  sprintf(tmpStr1, "%01.00f%%", liveData->params.auxPerc);
  monitoringRect(3, 0, 1, 1, tmpStr1, "AUX BAT.", (liveData->params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);

  // Aux amp
  sprintf(tmpStr1, (abs(liveData->params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), liveData->params.auxCurrentAmp);
  monitoringRect(3, 1, 1, 1, tmpStr1, "AUX AMPS",  (liveData->params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);

  // auxVoltage
  sprintf(tmpStr1, "%01.01f", liveData->params.auxVoltage);
  monitoringRect(3, 2, 1, 1, tmpStr1, "AUX VOLTS", (liveData->params.auxVoltage < 12.1 ? TFT_RED : (liveData->params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)), TFT_WHITE);

  // indoorTemperature
  sprintf(tmpStr1, "%01.01f", celsius2temperature(liveData->params.indoorTemperature));
  sprintf(tmpStr2, "IN/OUT%01.01f", celsius2temperature(liveData->params.outdoorTemperature));
  monitoringRect(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, TFT_WHITE);

  return true;
}

/**
   Speed + kwh/100km (Screen 2)
*/
bool drawSceneSpeed() {

  int32_t posx, posy;

  // HUD
  if (displayScreenSpeedHud) {

    // Change rotation to vertical & mirror
    if (tft.getRotation() != 6) {
      tft.setRotation(6);
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TR_DATUM); // top-right alignment
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // foreground, background text color

    // Draw speed
    tft.setTextSize((liveData->params.speedKmh > 99) ? 1 : 2);
    sprintf(tmpStr3, "0");
    if (liveData->params.speedKmh > 10)
      sprintf(tmpStr3, "%01.00f", km2distance(liveData->params.speedKmh));
    tft.drawString(tmpStr3, 240, 0, 8);

    // Draw power kWh/100km (>25kmh) else kW
    tft.setTextSize(1);
    if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0)
      sprintf(tmpStr3, "%01.01f", km2distance(liveData->params.batPowerKwh100));
    else
      sprintf(tmpStr3, "%01.01f", liveData->params.batPowerKw);
    tft.drawString(tmpStr3, 240, 150, 8);

    // Draw soc%
    sprintf(tmpStr3, "%01.00f", liveData->params.socPerc);
    tft.drawString(tmpStr3, 240 , 230, 8);

    // Cold gate cirlce
    tft.fillCircle(30, 280, 25, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);

    // Brake lights
    tft.fillRect(0, 310, 240, 10, (liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);

    return true;
  }

  //
  spr.fillRect(0, 36, 200, 160, TFT_DARKRED);

  posx = 320 / 2;
  posy = 40;
  spr.setTextDatum(TR_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_DARKRED);
  spr.setTextSize(2); // Size for small 5cix7 font
  sprintf(tmpStr3, "0");
  if (liveData->params.speedKmh > 10)
    sprintf(tmpStr3, "%01.00f", km2distance(liveData->params.speedKmh));
  spr.drawString(tmpStr3, 200, posy, 7);

  posy = 145;
  spr.setTextDatum(TR_DATUM); // Top center
  spr.setTextSize(1);
  if (liveData->params.speedKmh > 25 && liveData->params.batPowerKw < 0) {
    sprintf(tmpStr3, "%01.01f", km2distance(liveData->params.batPowerKwh100));
  } else {
    sprintf(tmpStr3, "%01.01f", liveData->params.batPowerKw);
  }
  spr.drawString(tmpStr3, 200, posy, 7);

  // Bottom 2 numbers with charged/discharged kWh from start
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  posx = 5;
  posy = 5;
  spr.setTextDatum(TL_DATUM);
  sprintf(tmpStr3, ((liveData->settings.distanceUnit == 'k') ? "%01.00fkm  " : "%01.00fmi  "), km2distance(liveData->params.odoKm));
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  if (liveData->params.motorRpm > -1) {
    spr.setTextDatum(TR_DATUM);
    sprintf(tmpStr3, "     %01.00frpm" , liveData->params.motorRpm);
    spr.drawString(tmpStr3, 320 - posx, posy, GFXFF);
  }

  // Bottom info
  // Cummulative regen/power
  posy = 240 - 5;
  sprintf(tmpStr3, "-%01.01f    ", liveData->params.cumulativeEnergyDischargedKWh - liveData->params.cumulativeEnergyDischargedKWhStart);
  spr.setTextDatum(BL_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  posx = 320 - 5;
  sprintf(tmpStr3, "    +%01.01f", liveData->params.cumulativeEnergyChargedKWh - liveData->params.cumulativeEnergyChargedKWhStart);
  spr.setTextDatum(BR_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);
  // Bat.power
  posx = 320 / 2;
  sprintf(tmpStr3, "   %01.01fkw   ", liveData->params.batPowerKw);
  spr.setTextDatum(BC_DATUM);
  spr.drawString(tmpStr3, posx, posy, GFXFF);

  // RIGHT INFO
  // Battery "cold gate" detection - red < 15C (43KW limit), <25 (blue - 55kW limit), green all ok
  spr.fillCircle(290, 60, 25, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  spr.setTextColor(TFT_WHITE, (liveData->params.batTempC >= 15) ? ((liveData->params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  spr.setFreeFont(&Roboto_Thin_24);
  spr.setTextDatum(MC_DATUM);
  sprintf(tmpStr3, "%01.00f", celsius2temperature(liveData->params.batTempC));
  spr.drawString(tmpStr3, 290, 60, GFXFF);
  // Brake lights
  spr.fillRect(210, 40, 40, 40, (liveData->params.brakeLights) ? TFT_RED : TFT_BLACK);

  // Soc%, bat.kWh
  spr.setFreeFont(&Orbitron_Light_32);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.setTextDatum(TR_DATUM);
  sprintf(tmpStr3, " %01.00f%%", liveData->params.socPerc);
  spr.drawString(tmpStr3, 320, 94, GFXFF);
  if (liveData->params.socPerc > 0) {
    float capacity = liveData->params.batteryTotalAvailableKWh * (liveData->params.socPerc / 100);
    // calibration for Niro/Kona, real available capacity is ~66.5kWh, 0-10% ~6.2kWh, 90-100% ~7.2kWh
    if (liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64) {
      capacity = (liveData->params.socPerc * 0.615) * (1 + (liveData->params.socPerc * 0.0008));
    }
    sprintf(tmpStr3, " %01.01f", capacity);
    spr.drawString(tmpStr3, 320, 129, GFXFF);
    spr.drawString("kWh", 320, 164, GFXFF);
  }

  return true;
}

/**
   Battery cells (Screen 3)
*/
bool drawSceneBatteryCells() {

  int32_t posx, posy;

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batHeaterC));
  drawSmallRect(0, 0, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batInletC));
  drawSmallRect(1, 0, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batModuleTempC[0]));
  drawSmallRect(0, 1, 1, 1, tmpStr1, "MO1", TFT_TEMP, (liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batModuleTempC[1]));
  drawSmallRect(1, 1, 1, 1, tmpStr1, "MO2", TFT_TEMP, (liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batModuleTempC[2]));
  drawSmallRect(2, 1, 1, 1, tmpStr1, "MO3", TFT_TEMP, (liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batModuleTempC[3]));
  drawSmallRect(3, 1, 1, 1, tmpStr1, "MO4", TFT_TEMP, (liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  // Ioniq (up to 12 cells)
  for (uint16_t i = 4; i < liveData->params.batModuleTempCount; i++) {
    if (liveData->params.batModuleTempC[i] == 0)
      continue;
    posx = (((i - 4) % 8) * 40);
    posy = ((floor((i - 4) / 8)) * 13) + 64;
    //spr.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
    spr.setTextSize(1); // Size for small 5x7 font
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(((liveData->params.batModuleTempC[i] >= 15) ? ((liveData->params.batModuleTempC[i] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED), TFT_BLACK);
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00fC" : "%01.01fF"), celsius2temperature(liveData->params.batModuleTempC[i]));
    spr.drawString(tmpStr1, posx + 4, posy, 2);
  }

  spr.setTextDatum(TL_DATUM); // Topleft
  spr.setTextSize(1); // Size for small 5x7 font

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < 98; i++) {
    if ((liveData->params.cellVoltage[i] < minVal || minVal == -1) && liveData->params.cellVoltage[i] != -1)
      minVal = liveData->params.cellVoltage[i];
    if ((liveData->params.cellVoltage[i] > maxVal || maxVal == -1) && liveData->params.cellVoltage[i] != -1)
      maxVal = liveData->params.cellVoltage[i];
    if (liveData->params.cellVoltage[i] > 0 && i > liveData->params.cellCount + 1)
      liveData->params.cellCount = i + 1;
  }

  // Draw cell matrix
  for (int i = 0; i < 98; i++) {
    if (liveData->params.cellVoltage[i] == -1)
      continue;
    posx = ((i % 8) * 40) + 4;
    posy = ((floor(i / 8) + (liveData->params.cellCount > 96 ? 0 : 1)) * 13) + 68;
    sprintf(tmpStr3, "%01.02f", liveData->params.cellVoltage[i]);
    spr.setTextColor(TFT_NAVY, TFT_BLACK);
    if (liveData->params.cellVoltage[i] == minVal && minVal != maxVal)
      spr.setTextColor(TFT_RED, TFT_BLACK);
    if (liveData->params.cellVoltage[i] == maxVal && minVal != maxVal)
      spr.setTextColor(TFT_GREEN, TFT_BLACK);
    spr.drawString(tmpStr3, posx, posy, 2);
  }

  return true;
}

/**
   drawPreDrawnChargingGraphs
   P = U.I
*/
bool drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, int mulY) {

  // Rapid gate
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 180 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_RAPIDGATE35);
  // Coldgate <5C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE0_5);
  // Coldgate 5-14C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 64 * mulX), zeroY - (/*I*/ 75 * /*U SOC*/ (64 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 65 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (65 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  spr.drawLine(zeroX + (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 40 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE5_14);
  // Coldgate 15-24C
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  spr.drawLine(zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 110 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_COLDGATE15_24);
  // Optimal
  spr.drawLine(zeroX + (/* SOC FROM */ 1 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 51 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 51 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (51 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 53 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 53 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (53 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 55 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 55 * mulX), zeroY - (/*I*/ 195 * /*U SOC*/ (55 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 57 * mulX), zeroY - (/*I*/ 200 * /*U SOC*/ (57 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 58 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (58 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 71 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 71 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (71 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 73 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 73 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (73 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 75 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 75 * mulX), zeroY - (/*I*/ 145 * /*U SOC*/ (75 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 77 * mulX), zeroY - (/*I*/ 150 * /*U SOC*/ (77 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 78 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (78 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 82 * mulX), zeroY - (/*I*/ 90 * /*U SOC*/ (82 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX + (/* SOC TO */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX + (/* SOC FROM */ 83 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (83 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 92 * mulX), zeroY - (/*I*/ 60 * /*U SOC*/ (92 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 95 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (95 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  spr.drawLine(zeroX +  (/* SOC FROM */ 98 * mulX), zeroY - (/*I*/ 35 * /*U SOC*/ (98 * 55 / 100 + 352) /**/ / 1000 * mulY),
               zeroX +  (/* SOC TO */ 100 * mulX), zeroY - (/*I*/ 15 * /*U SOC*/ (100 * 55 / 100 + 352) /**/ / 1000 * mulY), TFT_GRAPH_OPTIMAL25);
  // Triangles
  int x = zeroX;
  int y;
  if (liveData->params.batMaxC >= 35) {
    y = zeroY - (/*I*/ 180 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (liveData->params.batMinC >= 25) {
    y = zeroY - (/*I*/ 200 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (liveData->params.batMinC >= 15) {
    y = zeroY - (/*I*/ 150 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else if (liveData->params.batMinC >= 5) {
    y = zeroY - (/*I*/ 110 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  } else {
    y = zeroY - (/*I*/ 60 * /*U SOC*/ (1 * 55 / 100 + 352) /**/ / 1000 * mulY);
  }
  spr.fillTriangle(x + 5, y,  x , y - 5, x, y + 5, TFT_ORANGE);
}

/**
   Charging graph (Screen 4)
*/
bool drawSceneChargingGraph() {

  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3; // 100% = 300px;
  int mulY = 2; // 100kW = 200px
  int maxKw = 80;
  int posy = 0;
  uint16_t color;

  spr.fillSprite(TFT_BLACK);

  sprintf(tmpStr1, "%01.00f", liveData->params.socPerc);
  drawSmallRect(0, 0, 1, 1, tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerKw);
  drawSmallRect(1, 0, 1, 1, tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", liveData->params.batPowerAmp);
  drawSmallRect(2, 0, 1, 1, tmpStr1, "CURRENT A", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%03.00f", liveData->params.batVoltage);
  drawSmallRect(3, 0, 1, 1, tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batHeaterC));
  drawSmallRect(0, 1, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_RED);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batInletC));
  drawSmallRect(1, 1, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.batMinC));
  drawSmallRect(2, 1, 1, 1, tmpStr1, "BAT.MIN", (liveData->params.batMinC >= 15) ? ((liveData->params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(liveData->params.outdoorTemperature));
  drawSmallRect(3, 1, 1, 1, tmpStr1, "OUT.TEMP.", TFT_TEMP, TFT_CYAN);

  spr.setTextColor(TFT_SILVER, TFT_TEMP);

  for (int i = 0; i <= 10; i++) {
    color = TFT_DARKRED2;
    if (i == 0 || i == 5 || i == 10)
      color = TFT_DARKRED;
    spr.drawFastVLine(zeroX + (i * 10 * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
    /*if (i != 0 && i != 10) {
      sprintf(tmpStr1, "%d%%", i * 10);
      spr.setTextDatum(BC_DATUM);
      spr.drawString(tmpStr1, zeroX + (i * 10 * mulX),  zeroY - (maxKw * mulY), 2);
      }*/
    if (i <= (maxKw / 10)) {
      spr.drawFastHLine(zeroX, zeroY - (i * 10 * mulY), 100 * mulX, color);
      if (i > 0) {
        sprintf(tmpStr1, "%d", i * 10);
        spr.setTextDatum(ML_DATUM);
        spr.drawString(tmpStr1, zeroX + (100 * mulX) + 3, zeroY - (i * 10 * mulY), 2);
      }
    }
  }

  // Draw suggested curves
  if (liveData->settings.predrawnChargingGraphs == 1) {
    drawPreDrawnChargingGraphs(zeroX, zeroY, mulX, mulY);
  }

  // Draw realtime values
  for (int i = 0; i <= 100; i++) {
    if (liveData->params.chargingGraphBatMinTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMinTempC[i]*mulY), mulX, TFT_BLUE);
    if (liveData->params.chargingGraphBatMaxTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphBatMaxTempC[i]*mulY), mulX, TFT_BLUE);
    if (liveData->params.chargingGraphWaterCoolantTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphWaterCoolantTempC[i]*mulY), mulX, TFT_PURPLE);
    if (liveData->params.chargingGraphHeaterTempC[i] > -10)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphHeaterTempC[i]*mulY), mulX, TFT_RED);

    if (liveData->params.chargingGraphMinKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMinKw[i]*mulY), mulX, TFT_GREENYELLOW);
    if (liveData->params.chargingGraphMaxKw[i] > 0)
      spr.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (liveData->params.chargingGraphMaxKw[i]*mulY), mulX, TFT_YELLOW);
  }

  // Bat.module temperatures
  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextDatum(BL_DATUM);
  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "1=%01.00fC " : "1=%01.00fF "), celsius2temperature(liveData->params.batModuleTempC[0]));
  spr.setTextColor((liveData->params.batModuleTempC[0] >= 15) ? ((liveData->params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 0,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "2=%01.00fC " : "2=%01.00fF "), celsius2temperature(liveData->params.batModuleTempC[1]));
  spr.setTextColor((liveData->params.batModuleTempC[1] >= 15) ? ((liveData->params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 48,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "3=%01.00fC " : "3=%01.00fF "), celsius2temperature(liveData->params.batModuleTempC[2]));
  spr.setTextColor((liveData->params.batModuleTempC[2] >= 15) ? ((liveData->params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 96,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "4=%01.00fC " : "4=%01.00fF "), celsius2temperature(liveData->params.batModuleTempC[3]));
  spr.setTextColor((liveData->params.batModuleTempC[3] >= 15) ? ((liveData->params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  spr.drawString(tmpStr1, 144,  zeroY - (maxKw * mulY), 2);
  sprintf(tmpStr1, "ir %01.00fkOhm", liveData->params.isolationResistanceKOhm );

  // Bms max.regen/power available
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(tmpStr1, "xC=%01.00fkW ", liveData->params.availableChargePower);
  spr.drawString(tmpStr1, 192,  zeroY - (maxKw * mulY), 2);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(tmpStr1, "xD=%01.00fkW", liveData->params.availableDischargePower);
  spr.drawString(tmpStr1, 256,  zeroY - (maxKw * mulY), 2);

  //
  spr.setTextDatum(TR_DATUM);
  if (liveData->params.coolingWaterTempC != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "W=%01.00fC" : "W=%01.00fF"), celsius2temperature(liveData->params.coolingWaterTempC));
    spr.setTextColor(TFT_PURPLE, TFT_TEMP);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  spr.setTextColor(TFT_WHITE, TFT_TEMP);
  if (liveData->params.batFanFeedbackHz > 0) {
    sprintf(tmpStr1, "FF=%03.00fHz", liveData->params.batFanFeedbackHz);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.batFanStatus > 0) {
    sprintf(tmpStr1, "FS=%03.00f", liveData->params.batFanStatus);
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.coolantTemp1C != -1 && liveData->params.coolantTemp2C != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "C1/2:%01.00f/%01.00fC" : "C1/2:%01.00f/%01.00fF"), celsius2temperature(liveData->params.coolantTemp1C), celsius2temperature(liveData->params.coolantTemp2C));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempA != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "A=%01.00fC" : "W=%01.00fF"), celsius2temperature(liveData->params.bmsUnknownTempA));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempB != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "B=%01.00fC" : "W=%01.00fF"), celsius2temperature(liveData->params.bmsUnknownTempB));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempC != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "C=%01.00fC" : "W=%01.00fF"), celsius2temperature(liveData->params.bmsUnknownTempC));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (liveData->params.bmsUnknownTempD != -1) {
    sprintf(tmpStr1, ((liveData->settings.temperatureUnit == 'c') ? "D=%01.00fC" : "W=%01.00fF"), celsius2temperature(liveData->params.bmsUnknownTempD));
    spr.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }

  // Print charging time
  time_t diffTime = liveData->params.currentTime - liveData->params.chargingStartTime;
  if ((diffTime / 60) > 99)
    sprintf(tmpStr1, "%02d:%02d:%02d", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
  else
    sprintf(tmpStr1, "%02d:%02d", (diffTime / 60), diffTime % 60);
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(TFT_SILVER, TFT_BLACK);
  spr.drawString(tmpStr1, 0, zeroY - (maxKw * mulY), 2);

  // Debug info
  if (debugTmpCharging) {
    if (liveData->params.currentTime % 10 > 5) {
      int32_t posx, posy;
      String chHex, chHex2, chRef;
      uint8_t chByte;

      spr.setTextSize(1); // Size for small 5x7 font
      spr.setTextColor(TFT_SILVER, TFT_TEMP);
      spr.setTextDatum(TR_DATUM);
      // tft.fillRect(0, 240-104, 320, 104, TFT_BLACK);

      for (int i = 0; i < debugTmpChargingLast05.length() / 2; i++) {
        chHex = debugTmpChargingLast05.substring(i * 2, (i * 2) + 2);
        chHex2 = debugTmpChargingPrevious05.substring(i * 2, (i * 2) + 2);
        chRef = debugTmpChargingRef05.substring(i * 2, (i * 2) + 2);
        if (chRef.equals("--") || chRef.equals(chHex))
          continue;
        spr.setTextColor(((chHex.equals(chHex2)) ?  TFT_SILVER : TFT_GREEN), TFT_TEMP);
        chByte = liveData->hexToDec(chHex.c_str(), 1, false);
        posx = (((i) % 10) * 32) + 24;
        posy = ((floor((i) / 10)) * 13) + 240 - 104;
        sprintf(tmpStr1, "%03d", chByte);
        spr.drawString(tmpStr1, posx + 4, posy, 2);
      }
      for (int i = 0; i < debugTmpChargingLast06.length() / 2; i++) {
        chHex = debugTmpChargingLast06.substring(i * 2, (i * 2) + 2);
        chHex2 = debugTmpChargingPrevious06.substring(i * 2, (i * 2) + 2);
        chRef = debugTmpChargingRef06.substring(i * 2, (i * 2) + 2);
        if (chRef.equals("--") || chRef.equals(chHex))
          continue;
        spr.setTextColor(((chHex.equals(chHex2)) ?  TFT_SILVER : TFT_GREEN), TFT_TEMP);
        chByte = liveData->hexToDec(chHex.c_str(), 1, false);
        posx = (((i) % 10) * 32) + 24;
        posy = ((floor((i) / 10)) * 13) + 65 + 240 - 104;
        sprintf(tmpStr1, "%03d", chByte);
        spr.drawString(tmpStr1, posx + 4, posy, 2);
      }

      debugTmpChargingPrevious05 = debugTmpChargingLast05;
      debugTmpChargingPrevious06 = debugTmpChargingLast06;
    }
  }

  return true;
}

/**
  SOC 10% table (screen 5)
*/
bool drawSceneSoc10Table() {

  int zeroY = 2;
  float diffCec, diffCed, diffOdo = -1;
  float firstCed = -1, lastCed = -1, diffCed0to5 = 0;
  float firstCec = -1, lastCec = -1, diffCec0to5 = 0;
  float firstOdo = -1, lastOdo = -1, diffOdo0to5 = 0;
  float diffTime;
  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER, TFT_TEMP);
  spr.setTextDatum(TL_DATUM);
  spr.drawString("CONSUMPTION | DISCH.100%->4% SOC",  2, zeroY, 2);

  spr.setTextDatum(TR_DATUM);

  spr.drawString("dis./char.kWh", 128, zeroY + (1 * 15), 2);
  spr.drawString(((liveData->settings.distanceUnit == 'k') ? "km" : "mi"), 160, zeroY + (1 * 15), 2);
  spr.drawString("kWh100", 224, zeroY + (1 * 15), 2);
  spr.drawString("avg.speed", 310, zeroY + (1 * 15), 2);

  for (int i = 0; i <= 10; i++) {
    sprintf(tmpStr1, "%d%%", (i == 0) ? 5 : i * 10);
    spr.drawString(tmpStr1, 32, zeroY + ((12 - i) * 15), 2);

    firstCed = (liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : firstCed;
    lastCed = (lastCed == -1 && liveData->params.soc10ced[i] != -1) ? liveData->params.soc10ced[i] : lastCed;
    firstCec = (liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : firstCec;
    lastCec = (lastCec == -1 && liveData->params.soc10cec[i] != -1) ? liveData->params.soc10cec[i] : lastCec;
    firstOdo = (liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : firstOdo;
    lastOdo = (lastOdo == -1 && liveData->params.soc10odo[i] != -1) ? liveData->params.soc10odo[i] : lastOdo;

    if (i != 10) {
      diffCec = (liveData->params.soc10cec[i + 1] != -1 && liveData->params.soc10cec[i] != -1) ? (liveData->params.soc10cec[i] - liveData->params.soc10cec[i + 1]) : 0;
      diffCed = (liveData->params.soc10ced[i + 1] != -1 && liveData->params.soc10ced[i] != -1) ? (liveData->params.soc10ced[i + 1] - liveData->params.soc10ced[i]) : 0;
      diffOdo = (liveData->params.soc10odo[i + 1] != -1 && liveData->params.soc10odo[i] != -1) ? (liveData->params.soc10odo[i] - liveData->params.soc10odo[i + 1]) : -1;
      diffTime = (liveData->params.soc10time[i + 1] != -1 && liveData->params.soc10time[i] != -1) ? (liveData->params.soc10time[i] - liveData->params.soc10time[i + 1]) : -1;
      if (diffCec != 0) {
        sprintf(tmpStr1, "+%01.01f", diffCec);
        spr.drawString(tmpStr1, 128, zeroY + ((12 - i) * 15), 2);
        diffCec0to5 = (i == 0) ? diffCec : diffCec0to5;
      }
      if (diffCed != 0) {
        sprintf(tmpStr1, "%01.01f", diffCed);
        spr.drawString(tmpStr1, 80, zeroY + ((12 - i) * 15), 2);
        diffCed0to5 = (i == 0) ? diffCed : diffCed0to5;
      }
      if (diffOdo != -1) {
        sprintf(tmpStr1, "%01.00f", km2distance(diffOdo));
        spr.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
        diffOdo0to5 = (i == 0) ? diffOdo : diffOdo0to5;
        if (diffTime > 0) {
          sprintf(tmpStr1, "%01.01f", km2distance(diffOdo) / (diffTime / 3600));
          spr.drawString(tmpStr1, 310, zeroY + ((12 - i) * 15), 2);
        }
      }
      if (diffOdo > 0 && diffCed != 0) {
        sprintf(tmpStr1, "%01.1f", (-diffCed * 100.0 / km2distance(diffOdo)));
        spr.drawString(tmpStr1, 224, zeroY + ((12 - i) * 15), 2);
      }
    }

    if (diffOdo == -1 && liveData->params.soc10odo[i] != -1) {
      sprintf(tmpStr1, "%01.00f", km2distance(liveData->params.soc10odo[i]));
      spr.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
    }
  }

  spr.drawString("0%", 32, zeroY + (13 * 15), 2);
  spr.drawString("0-5% is calculated (same) as 5-10%", 310, zeroY + (13 * 15), 2);

  spr.drawString("TOT.", 32, zeroY + (14 * 15), 2);
  diffCed = (lastCed != -1 && firstCed != -1) ? firstCed - lastCed + diffCed0to5 : 0;
  sprintf(tmpStr1, "%01.01f", diffCed);
  spr.drawString(tmpStr1, 80, zeroY + (14 * 15), 2);
  diffCec = (lastCec != -1 && firstCec != -1) ? lastCec - firstCec + diffCec0to5 : 0;
  sprintf(tmpStr1, "+%01.01f", diffCec);
  spr.drawString(tmpStr1, 128, zeroY + (14 * 15), 2);
  diffOdo = (lastOdo != -1 && firstOdo != -1) ? lastOdo - firstOdo + diffOdo0to5 : 0;
  sprintf(tmpStr1, "%01.00f", km2distance(diffOdo));
  spr.drawString(tmpStr1, 160, zeroY + (14 * 15), 2);
  sprintf(tmpStr1, "AVAIL.CAP: %01.01f kWh", -diffCed - diffCec);
  spr.drawString(tmpStr1, 310, zeroY + (14 * 15), 2);

  return true;
}

/**
  DEBUG screen
*/
bool drawSceneDebug() {

  int32_t posx, posy;
  String chHex, chHex2;
  uint8_t chByte;

  spr.setTextSize(1); // Size for small 5x7 font
  spr.setTextColor(TFT_SILVER, TFT_TEMP);
  spr.setTextDatum(TL_DATUM);
  spr.drawString(debugAtshRequest, 0, 0, 2);
  spr.drawString(debugCommandRequest, 128, 0, 2);
  spr.drawString(liveData->commandRequest, 256, 0, 2);
  spr.setTextDatum(TR_DATUM);

  for (int i = 0; i < debugLastString.length() / 2; i++) {
    chHex = debugLastString.substring(i * 2, (i * 2) + 2);
    chHex2 = debugPreviousString.substring(i * 2, (i * 2) + 2);
    spr.setTextColor(((chHex.equals(chHex2)) ?  TFT_SILVER : TFT_GREEN), TFT_TEMP);
    chByte = liveData->hexToDec(chHex.c_str(), 1, false);
    posx = (((i) % 10) * 32) + 24;
    posy = ((floor((i) / 10)) * 32) + 24;
    sprintf(tmpStr1, "%03d", chByte);
    spr.drawString(tmpStr1, posx + 4, posy, 2);

    spr.setTextColor(TFT_YELLOW, TFT_TEMP);
    sprintf(tmpStr1, "%c", (char)chByte);
    spr.drawString(tmpStr1, posx + 4, posy + 13, 2);
  }

  debugPreviousString = debugLastString;

  return true;
}

/**
   Modify caption
*/
String menuItemCaption(int16_t menuItemId, String title) {

  String prefix = "", suffix = "";

  if (menuItemId == 10) // Version
    suffix = APP_VERSION;

  if (menuItemId == 401) // distance
    suffix = (liveData->settings.distanceUnit == 'k') ? "[km]" : "[mi]";
  if (menuItemId == 402) // temperature
    suffix = (liveData->settings.temperatureUnit == 'c') ? "[C]" : "[F]";
  if (menuItemId == 403) // pressure
    suffix = (liveData->settings.pressureUnit == 'b') ? "[bar]" : "[psi]";

  title = ((prefix == "") ? "" : prefix + " ") + title + ((suffix == "") ? "" : " " + suffix);
  return title;
}

/**
  Display menu
*/
bool showMenu() {

  uint16_t posY = 0, tmpCurrMenuItem = 0;

  menuVisible = true;

  spr.fillSprite(TFT_BLACK);
  spr.setTextDatum(TL_DATUM);
  spr.setFreeFont(&Roboto_Thin_24);

  // Page scroll
  uint8_t visibleCount = (int)(tft.height() / spr.fontHeight());
  if (menuItemSelected >= menuItemOffset + visibleCount)
    menuItemOffset = menuItemSelected - visibleCount + 1;
  if (menuItemSelected < menuItemOffset)
    menuItemOffset = menuItemSelected;

  // Print items
  for (uint16_t i = 0; i < menuItemsCount; ++i) {
    if (menuCurrent == menuItems[i].parentId) {
      if (tmpCurrMenuItem >= menuItemOffset) {
        spr.fillRect(0, posY, 320, spr.fontHeight() + 2, (menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
        spr.setTextColor((menuItemSelected == tmpCurrMenuItem) ? TFT_WHITE : TFT_WHITE, (menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
        spr.drawString(menuItemCaption(menuItems[i].id, menuItems[i].title), 0, posY + 2, GFXFF);
        posY += spr.fontHeight();
      }
      tmpCurrMenuItem++;
    }
  }

  spr.pushSprite(0, 0);

  return true;
}

/**
   Hide menu
*/
bool hideMenu() {

  menuVisible = false;
  menuCurrent = 0;
  menuItemSelected = 0;
  redrawScreen();

  return false;
}

/**
  Move in menu with left/right button
*/
bool menuMove(bool forward) {

  if (forward) {
    uint16_t tmpCount = 0;
    for (uint16_t i = 0; i < menuItemsCount; ++i) {
      if (menuCurrent == menuItems[i].parentId) {
        tmpCount++;
      }
    }
    menuItemSelected = (menuItemSelected >= tmpCount - 1 ) ? tmpCount - 1 : menuItemSelected + 1;
  } else {
    menuItemSelected = (menuItemSelected <= 0) ? 0 : menuItemSelected - 1;
  }
  showMenu();

  return true;
}

/**
   Enter menu item
*/
bool menuItemClick() {

  // Locate menu item for meta data
  MENU_ITEM tmpMenuItem;
  uint16_t tmpCurrMenuItem = 0;
  for (uint16_t i = 0; i < menuItemsCount; ++i) {
    if (menuCurrent == menuItems[i].parentId) {
      if (menuItemSelected == tmpCurrMenuItem) {
        tmpMenuItem = menuItems[i];
        break;
      }
      tmpCurrMenuItem++;
    }
  }

  // Exit menu, parent level menu, open item
  if (menuItemSelected == 0) {
    // Exit menu
    if (tmpMenuItem.parentId == 0 && tmpMenuItem.id == 0) {
      menuVisible = false;
      redrawScreen();
    } else {
      // Parent menu
      menuCurrent = tmpMenuItem.targetParentId;
      showMenu();
    }
    return true;
  } else {
    Serial.println(tmpMenuItem.id);
    // Device list
    if (tmpMenuItem.id > 10000 && tmpMenuItem.id < 10100) {
      strlcpy((char*)liveData->settings.obdMacAddress, (char*)tmpMenuItem.obdMacAddress, 20);
      Serial.print("Selected adapter MAC address ");
      Serial.println(liveData->settings.obdMacAddress);
      saveSettings();
      ESP.restart();
    }
    // Other menus
    switch (tmpMenuItem.id) {
      // Set vehicle type
      case 101: liveData->settings.carType = CAR_KIA_ENIRO_2020_64; break;
      case 102: liveData->settings.carType = CAR_HYUNDAI_KONA_2020_64; break;
      case 103: liveData->settings.carType = CAR_HYUNDAI_IONIQ_2018; break;
      case 104: liveData->settings.carType = CAR_KIA_ENIRO_2020_39; break;
      case 105: liveData->settings.carType = CAR_HYUNDAI_KONA_2020_39; break;
      case 107: liveData->settings.carType = CAR_DEBUG_OBD2_KIA; break;
      // Screen orientation
      case 3011: liveData->settings.displayRotation = 1; tft.setRotation(liveData->settings.displayRotation); break;
      case 3012: liveData->settings.displayRotation = 3; tft.setRotation(liveData->settings.displayRotation); break;
      // Default screen
      case 3021: liveData->settings.defaultScreen = 1; break;
      case 3022: liveData->settings.defaultScreen = 2; break;
      case 3023: liveData->settings.defaultScreen = 3; break;
      case 3024: liveData->settings.defaultScreen = 4; break;
      case 3025: liveData->settings.defaultScreen = 5; break;
      // Debug screen off/on
      case 3031: liveData->settings.debugScreen = 0; break;
      case 3032: liveData->settings.debugScreen = 1; break;
      // Lcd brightness
      case 3041: liveData->settings.lcdBrightness = 0; break;
      case 3042: liveData->settings.lcdBrightness = 20; break;
      case 3043: liveData->settings.lcdBrightness = 50; break;
      case 3044: liveData->settings.lcdBrightness = 100; break;
      // Pre-drawn charg.graphs off/on
      case 3051: liveData->settings.predrawnChargingGraphs = 0; break;
      case 3052: liveData->settings.predrawnChargingGraphs = 1; break;
      // Distance
      case 4011: liveData->settings.distanceUnit = 'k'; break;
      case 4012: liveData->settings.distanceUnit = 'm'; break;
      // Temperature
      case 4021: liveData->settings.temperatureUnit = 'c'; break;
      case 4022: liveData->settings.temperatureUnit = 'f'; break;
      // Pressure
      case 4031: liveData->settings.pressureUnit = 'b'; break;
      case 4032: liveData->settings.pressureUnit = 'p'; break;
      // Pair ble device
      case 2: startBleScan(); return false;
      // Reset settings
      case 8: resetSettings(); hideMenu(); return false;
      // Save settings
      case 9: saveSettings(); break;
      // Version
      case 10: hideMenu(); return false;
      // Shutdown
      case 11: shutdownDevice(); return false;
      default:
        // Submenu
        menuCurrent = tmpMenuItem.id;
        menuItemSelected = 0;
        showMenu();
        return true;
    }

    // close menu
    hideMenu();
  }

  return true;
}

/**
   Redraw screen
*/
bool redrawScreen() {

  if (menuVisible) {
    return false;
  }

  // Lights not enabled
  if (!testDataMode && liveData->params.forwardDriveMode && !liveData->params.headLights && !liveData->params.dayLights) {
    spr.fillSprite(TFT_RED);
    spr.setFreeFont(&Orbitron_Light_32);
    spr.setTextColor(TFT_WHITE, TFT_RED);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("! LIGHTS OFF !", 160, 120, GFXFF);
    spr.pushSprite(0, 0);

    return true;
  }

  spr.fillSprite(TFT_BLACK);

  // 1. Auto mode = >5kpm Screen 3 - speed, other wise basic Screen2 - Main screen, if charging then Screen 5 Graph
  if (displayScreen == SCREEN_AUTO) {
    if (liveData->params.speedKmh > 5) {
      if (displayScreenAutoMode != 3) {
        displayScreenAutoMode = 3;
      }
      drawSceneSpeed();
    } else if (liveData->params.batPowerKw > 1) {
      if (displayScreenAutoMode != 5) {
        displayScreenAutoMode = 5;
      }
      drawSceneChargingGraph();
    } else {
      if (displayScreenAutoMode != 2) {
        displayScreenAutoMode = 2;
      }
      drawSceneMain();
    }
  }
  // 2. Main screen
  if (displayScreen == SCREEN_DASH) {
    drawSceneMain();
  }
  // 3. Big speed + kwh/100km
  if (displayScreen == SCREEN_SPEED) {
    drawSceneSpeed();
  }
  // 4. Battery cells
  if (displayScreen == SCREEN_CELLS) {
    drawSceneBatteryCells();
  }
  // 5. Charging graph
  if (displayScreen == SCREEN_CHARGING) {
    drawSceneChargingGraph();
  }
  // 6. SOC10% table (CEC-CED)
  if (displayScreen == SCREEN_SOC10) {
    drawSceneSoc10Table();
  }
  // 7. DEBUG SCREEN
  if (displayScreen == SCREEN_DEBUG) {
    drawSceneDebug();
  }

  if (!displayScreenSpeedHud) {
    // BLE not connected
    if (!bleConnected && bleConnect) {
      // Print message
      spr.setTextSize(1);
      spr.setTextColor(TFT_WHITE, TFT_BLACK);
      spr.setTextDatum(TL_DATUM);
      spr.drawString("BLE4 OBDII not connected...", 0, 180, 2);
      spr.drawString("Press middle button to menu.", 0, 200, 2);
      spr.drawString(APP_VERSION, 0, 220, 2);
    }

    spr.pushSprite(0, 0);
  }

  return true;
}

/**
   Do next AT command from queue
*/
bool doNextAtCommand() {

  // Restart loop with AT commands
  if (liveData->commandQueueIndex >= liveData->commandQueueCount) {
    liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
    redrawScreen();
  }

  // Send AT command to obd
  liveData->commandRequest = liveData->commandQueue[liveData->commandQueueIndex];
  if (liveData->commandRequest.startsWith("ATSH")) {
    liveData->currentAtshRequest = liveData->commandRequest;
  }

  Serial.print(">>> ");
  Serial.println(liveData->commandRequest);
  String tmpStr = liveData->commandRequest + "\r";
  pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  liveData->commandQueueIndex++;

  return true;
}

/**
  Parse result from OBD, create single line liveData->responseRowMerged
*/
bool parseRow() {

  // Simple 1 line responses
  Serial.print("");
  Serial.println(liveData->responseRow);

  // Merge 0:xxxx 1:yyyy 2:zzzz to single xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    if (liveData->responseRow.charAt(0) == '0') {
      liveData->responseRowMerged = "";
    }
    liveData->responseRowMerged += liveData->responseRow.substring(2);
  }

  return true;
}

/**
  Parse merged row (after merge completed)
*/
bool parseRowMerged() {

  Serial.print("merged:");
  Serial.println(liveData->responseRowMerged);

  // Catch output for debug screen
  if (displayScreen == SCREEN_DEBUG) {
    if (debugCommandIndex == liveData->commandQueueIndex) {
      debugAtshRequest = liveData->currentAtshRequest;
      debugCommandRequest = liveData->commandRequest;
      debugLastString  = liveData->responseRowMerged;
    }
  }
  if (debugTmpCharging && liveData->currentAtshRequest.equals("ATSH7E4")) {
    if (liveData->commandRequest.equals("220105"))
      debugTmpChargingLast05 = liveData->responseRowMerged;
    if (liveData->commandRequest.equals("220106"))
      debugTmpChargingLast06 = liveData->responseRowMerged;
  }

  // Parse by selected car interface
  carInterface->parseRowMerged();

  return true;
}

/**
   Parse test data
*/
bool testData() {

  testDataMode = true; // skip lights off message
  carInterface->loadTestData();
  redrawScreen();
  
  return true;
}

/**
  BLE callbacks
*/
class MyClientCallback : public BLEClientCallbacks {

    /**
      On connect
    */
    void onConnect(BLEClient* pclient) {
      Serial.println("onConnect");
    }

    /**
      On disconnect
    */
    void onDisconnect(BLEClient* pclient) {
      //connected = false;
      Serial.println("onDisconnect");
      displayMessage("BLE disconnected", "");
    }
};

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    /**
      Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {

      Serial.print("BLE advertised device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      Serial.println(advertisedDevice.getAddress().toString().c_str());

      // Add to device list (max. 9 devices allowed yet)
      String tmpStr;
      if (scanningDeviceIndex < 10/* && advertisedDevice.haveServiceUUID()*/) {
        for (uint16_t i = 0; i < menuItemsCount; ++i) {
          if (menuItems[i].id == 10001 + scanningDeviceIndex) {
            tmpStr = advertisedDevice.toString().c_str();
            tmpStr.replace("Name: ", "");
            tmpStr.replace("Address: ", "");
            tmpStr.toCharArray(menuItems[i].title, 48);
            tmpStr = advertisedDevice.getAddress().toString().c_str();
            tmpStr.toCharArray(menuItems[i].obdMacAddress, 18);
          }
        }
        scanningDeviceIndex++;
      }
      /*
        if (advertisedDevice.getServiceDataUUID().toString() != "<NULL>") {
        Serial.print("ServiceDataUUID: ");
        Serial.println(advertisedDevice.getServiceDataUUID().toString().c_str());
        if (advertisedDevice.getServiceUUID().toString() != "<NULL>") {
          Serial.print("ServiceUUID: ");
          Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
        }
        }*/

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(liveData->settings.serviceUUID)) &&
          (strcmp(advertisedDevice.getAddress().toString().c_str(), liveData->settings.obdMacAddress) == 0)) {
        Serial.println("Stop scanning. Found my BLE device.");
        BLEDevice::getScan()->stop();
        foundMyBleDevice = new BLEAdvertisedDevice(advertisedDevice);
      }
    }
};

/**
  BLE Security
*/
class MySecurity : public BLESecurityCallbacks {

    uint32_t onPassKeyRequest() {
      Serial.printf("Pairing password: %d \r\n", PIN);
      return PIN;
    }

    void onPassKeyNotify(uint32_t pass_key) {
      Serial.printf("onPassKeyNotify\r\n");
    }

    bool onConfirmPIN(uint32_t pass_key) {
      Serial.printf("onConfirmPIN\r\n");
      return true;
    }

    bool onSecurityRequest() {
      Serial.printf("onSecurityRequest\r\n");
      return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
      if (auth_cmpl.success) {
        Serial.printf("onAuthenticationComplete\r\n");
      } else {
        Serial.println("Auth failure. Incorrect PIN?");
        bleConnect = false;
      }
    }
};

/**
   Ble notification callback
*/
static void notifyCallback (BLERemoteCharacteristic * pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  char ch;

  // Parse multi line response to single lines
  liveData->responseRow = "";
  for (int i = 0; i <= length; i++) {
    ch = pData[i];
    if (ch == '\r'  || ch == '\n' || ch == '\0') {
      if (liveData->responseRow != "")
        parseRow();
      liveData->responseRow = "";
    } else {
      liveData->responseRow += ch;
      if (liveData->responseRow == ">") {
        if (liveData->responseRowMerged != "") {
          parseRowMerged();
        }
        liveData->responseRowMerged = "";
        liveData->canSendNextAtCommand = true;
      }
    }
  }
}

/**
   Do connect BLE with server (OBD device)
*/
bool connectToServer(BLEAddress pAddress) {

  displayMessage(" > Connecting device", "");

  Serial.print("bleConnect ");
  Serial.println(pAddress.toString().c_str());
  displayMessage(" > Connecting device - init", pAddress.toString().c_str());

  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); //
  pSecurity->setCapability(ESP_IO_CAP_KBDISP);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  displayMessage(" > Connecting device", pAddress.toString().c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  if (pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) Serial.println("bleConnected");
  Serial.println(" - bleConnected to server");

  displayMessage(" > Connecting device", "Connecting service...");
  // Remote service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(liveData->settings.serviceUUID));
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(liveData->settings.serviceUUID);
    displayMessage(" > Connecting device", "Unable to find service");
    return false;
  }
  Serial.println(" - Found our service");

  // Get characteristics
  displayMessage(" > Connecting device", "Connecting TxUUID...");
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charTxUUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(liveData->settings.charTxUUID);//.toString().c_str());
    displayMessage(" > Connecting device", "Unable to find TxUUID");
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Get characteristics
  displayMessage(" > Connecting device", "Connecting RxUUID...");
  pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charRxUUID));
  if (pRemoteCharacteristicWrite == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(liveData->settings.charRxUUID);//.toString().c_str());
    displayMessage(" > Connecting device", "Unable to find RxUUID");
    return false;
  }
  Serial.println(" - Found our characteristic write");

  displayMessage(" > Connecting device", "Register callbacks...");
  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canNotify()) {
    Serial.println(" - canNotify");
    //pRemoteCharacteristic->registerForNotify(notifyCallback);
    if (pRemoteCharacteristic->canIndicate()) {
      Serial.println(" - canIndicate");
      const uint8_t indicationOn[] = {0x2, 0x0};
      //const uint8_t indicationOff[] = {0x0,0x0};
      pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)indicationOn, 2, true);
      //pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notifyOff,2,true);
      pRemoteCharacteristic->registerForNotify(notifyCallback, false);
      delay(200);
    }
  }

  displayMessage(" > Connecting device", "Done...");
  if (pRemoteCharacteristicWrite->canWrite()) {
    Serial.println(" - canWrite");
  }

  return true;
}

/**
   Start ble scan
*/
bool startBleScan() {

  foundMyBleDevice = NULL;
  scanningDeviceIndex = 0;

  displayMessage(" > Scanning BLE4 devices", "40 seconds");

  // Start scanning
  Serial.println("Scanning BLE devices...");
  Serial.print("Looking for ");
  Serial.println(liveData->settings.obdMacAddress);
  BLEScanResults foundDevices = pBLEScan->start(40, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  sprintf(tmpStr1, "Found %d devices", foundDevices.getCount());
  displayMessage(" > Scanning BLE4 devices", tmpStr1);

  // Scan devices from menu, show list of devices
  if (menuItemSelected == 2) {
    Serial.println("Display menu with devices");
    menuVisible = true;
    menuCurrent = 9999;
    menuItemSelected = 0;
    showMenu();
  } else {
    // Redraw screen
    if (foundMyBleDevice == NULL) {
      displayMessage("Device not found", "Middle button - menu");
    } else {
      redrawScreen();
    }
  }

  return true;
}

/**
  SIM800L
*/
#ifdef SIM800L_ENABLED
bool sim800lSetup() {
  const char APN[] = "internet.t-mobile.cz";
  
  Serial.println("Setting SIM800L module");
  SoftwareSerial* serial = new SoftwareSerial(SIM800L_RX, SIM800L_TX);
  serial->begin(9600);
  sim800l = new SIM800L((Stream *)serial, SIM800L_RST, 512 , 512);

  bool sim800l_ready = sim800l->isReady();
  for(uint8_t i = 0; i < 5 && !sim800l_ready; i++) {
    Serial.println("Problem to initialize SIM800L module, retry in 1 sec");
    delay(1000);
    sim800l_ready = sim800l->isReady();
  }

  if(!sim800l_ready) {
    params.sim800l_enabled = false;
    Serial.println("Problem to initialize SIM800L module - module disabled");
  } else {
    Serial.println("SIM800L - Setup Complete!");
    Serial.println("Setting GPRS connection");

    bool sim800l_gprs = sim800l->setupGPRS(APN);
    for(uint8_t i = 0; i < 5 && !sim800l_gprs; i++) {
      Serial.println("Problem to set GPRS connection, retry in 1 sec");
      delay(1000);
      sim800l_gprs = sim800l->setupGPRS(APN);
    }

    if(sim800l_gprs) {
      Serial.println("GPRS OK");
    } else {
      Serial.println("Problem to set GPRS");
    }
  }
  
  return true;  
}

bool sendDataViaGPRS() {
  Serial.println("Sending data via GPRS");

  NetworkRegistration network = sim800l->getRegistrationStatus();
  if(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    Serial.println("SIM800L module not connected to network!");
    return false;
  }

  bool connected = sim800l->connectGPRS();
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    connected = sim800l->connectGPRS();
  }
 
  if(!connected) {
    Serial.println("GPRS not connected! Reseting SIM800L module!");
    sim800l->reset();
    sim800lSetup();
    
    return false;
  }

  Serial.println("Start HTTP POST...");

  StaticJsonDocument<250> jsonData;

  jsonData["soc"] = params.socPerc;
  jsonData["soh"] = params.sohPerc;
  jsonData["batK"] = params.batPowerKw;
  jsonData["batA"] = params.batPowerAmp;
  jsonData["batV"] = params.batVoltage;
  jsonData["auxV"] = params.auxVoltage;
  jsonData["MinC"] = params.batMinC;
  jsonData["MaxC"] = params.batMaxC;
  jsonData["InlC"] = params.batInletC;
  jsonData["fan"] = params.batFanStatus;
  jsonData["cumCh"] = params.cumulativeEnergyChargedKWh;
  jsonData["cumD"] = params.cumulativeEnergyDischargedKWh;

  char payload[200];
  serializeJson(jsonData, payload);

  uint16_t rc = sim800l->doPost("http://api.example.com", "application/json", payload, 10000, 10000);
  if(rc == 200) {
    Serial.println(F("HTTP POST successful"));
  } else {
    // Failed...
    Serial.print(F("HTTP POST error: "));
    Serial.println(rc);
  }

  sim800l->disconnectGPRS();
  
  return true;
}
#endif //SIM800L_ENABLED

/**
  Setup device
*/
void setup(void) {

  // Serial console, init structures
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Booting device...");

  //
  liveData = new LiveData();
  initStructure();
  loadSettings();

  // Set button pins for input
  pinMode(BUTTON_MIDDLE, INPUT);
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

#ifdef BOARD_M5STACK_CORE
  // mute speaker
  Serial.println("Mute speaker for m5stack");
  dacWrite(SPEAKER_PIN, 0);
#endif // BOARD_M5STACK_C
  // Init display
  Serial.println("Init TFT display");
  tft.begin();

#ifdef INVERT_DISPLAY
  tft.invertDisplay(true);
#endif // INVERT_DISPLAY

  tft.setRotation(liveData->settings.displayRotation);
  analogWrite(TFT_BL, (liveData->settings.lcdBrightness == 0) ? 100 : liveData->settings.lcdBrightness);
  tft.fillScreen(TFT_BLACK);

  bool psramUsed = false; // 320x240 16bpp sprites requires psram
#if defined (ESP32) && defined (CONFIG_SPIRAM_SUPPORT)
  if (psramFound())
    psramUsed = true;
#endif
  spr.setColorDepth((psramUsed) ? 16 : 8);
  spr.createSprite(320, 240);
  redrawScreen();

  // Init time library
  struct timeval tv;
  tv.tv_sec = 1589011873;
  settimeofday(&tv, NULL);

  struct tm now;
  getLocalTime(&now, 0);
  liveData->params.chargingStartTime = liveData->params.currentTime = mktime(&now);

  // Show test data on right button during boot device
  displayScreen = liveData->settings.defaultScreen;
  if (digitalRead(BUTTON_RIGHT) == LOW) {
    testData();
  }

  // Init SDCARD
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCLK)) {
    Serial.println("SDCARD initialization failed!");
  } else {
    Serial.println("SDCARD initialization done.");
  }
  /*spiSD.begin(SD_SCLK,SD_MISO,SD_MOSI,SD_CS);
    if(!SD.begin( SD_CS, spiSD, 27000000)){
    Serial.println("SDCARD initialization failed!");
    } else {
    Serial.println("SDCARD initialization done.");
    }*/

  // Start BLE connection
  line = "";
  Serial.println("Start BLE with PIN auth");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we have detected a new device.
  // Specify that we want active scanning and start the scan to run for 10 seconds.
  Serial.println("Setup BLE scan");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  // Skip BLE scan if middle button pressed
  if (digitalRead(BUTTON_MIDDLE) == HIGH && digitalRead(BUTTON_RIGHT) == HIGH) {
    startBleScan();
  }

#ifdef SIM800L_ENABLED
  sim800lSetup();
#endif //SIM800L_ENABLED

  // End
  Serial.println("Device setup completed");
}

/**
  Loop
*/
void loop() {

  // Connect BLE device
  if (bleConnect == true && foundMyBleDevice != NULL) {
    pServerAddress = new BLEAddress(liveData->settings.obdMacAddress);
    if (connectToServer(*pServerAddress)) {

      bleConnected = true;
      bleConnect = false;

      Serial.println("We are now connected to the BLE device.");

      // Print message
      displayMessage(" > Processing init AT cmds", "");

      // Serve first command (ATZ)
      doNextAtCommand();
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
  }

  // Send command from TTY to OBD2
  if (bleConnected) {
    if (Serial.available()) {
      ch = Serial.read();
      line = line + ch;
      if (ch == '\r' || ch == '\n') {
        Serial.println(line);
        pRemoteCharacteristicWrite->writeValue(line.c_str(), line.length());
        line = "";
      }
    }

    // Can send next command from queue to OBD
    if (liveData->canSendNextAtCommand) {
      liveData->canSendNextAtCommand = false;
      doNextAtCommand();
    }
  }

#ifdef SIM800L_ENABLED
  if(params.lastDataSent + SIM800L_TIMER < params.currentTime && params.sim800l_enabled) {
    sendDataViaGPRS();
    params.lastDataSent = params.currentTime;
  }
#endif //SIM800L_ENABLED

  ///////////////////////////////////////////////////////////////////////
  // Handle buttons
  // MIDDLE - menu select
  if (digitalRead(BUTTON_MIDDLE) == HIGH) {
    btnMiddlePressed = false;
  } else {
    if (!btnMiddlePressed) {
      btnMiddlePressed = true;
      tft.setRotation(liveData->settings.displayRotation);
      if (menuVisible) {
        menuItemClick();
      } else {
        showMenu();
      }
    }
  }
  // LEFT - screen rotate, menu
  if (digitalRead((liveData->settings.displayRotation == 1) ? BUTTON_RIGHT : BUTTON_LEFT) == HIGH) {
    btnLeftPressed = false;
  } else {
    if (!btnLeftPressed) {
      btnLeftPressed = true;
      tft.setRotation(liveData->settings.displayRotation);
      // Menu handling
      if (menuVisible) {
        menuMove(false);
      } else {
        displayScreen++;
        if (displayScreen > displayScreenCount - (liveData->settings.debugScreen == 0) ? 1 : 0)
          displayScreen = 0; // rotate screens
        // Turn off display on screen 0
        analogWrite(TFT_BL, (displayScreen == SCREEN_BLANK) ? 0 : (liveData->settings.lcdBrightness == 0) ? 100 : liveData->settings.lcdBrightness);
        redrawScreen();
      }
    }
  }
  // RIGHT - menu, debug screen rotation
  if (digitalRead((liveData->settings.displayRotation == 1) ? BUTTON_LEFT : BUTTON_RIGHT) == HIGH) {
    btnRightPressed = false;
  } else {
    if (!btnRightPressed) {
      btnRightPressed = true;
      tft.setRotation(liveData->settings.displayRotation);
      // Menu handling
      if (menuVisible) {
        menuMove(true);
      } else {
        // doAction
        if (displayScreen == SCREEN_SPEED) {
          displayScreenSpeedHud = !displayScreenSpeedHud;
          redrawScreen();
        }
        if (liveData->settings.debugScreen == 1 && displayScreen == SCREEN_DEBUG) {
          debugCommandIndex = (debugCommandIndex >= liveData->commandQueueCount) ? liveData->commandQueueLoopFrom : debugCommandIndex + 1;
          redrawScreen();
        }

      }
    }
  }

  // currentTime & 1ms delay
  struct tm now;
  getLocalTime(&now, 0);
  liveData->params.currentTime = mktime(&now);
  // Shutdown when car is off
  if (liveData->params.automatickShutdownTimer != 0 && liveData->params.currentTime - liveData->params.automatickShutdownTimer > 5)
    shutdownDevice();
}
