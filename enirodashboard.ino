/*

  KIA eNiro Dashboard 1.7.1, 2020-10-18
  !! working only with OBD BLE 4.0 adapters
  !! Supported adapter is  Vgate ICar Pro (must be BLE4.0 version)
  !! Not working with standard BLUETOOTH 3 adapters

  IMPORTANT Replace HM_MAC, serviceUUID, charTxUUID, charRxUUID as described below

  !! How to obtain MAC + 3x UUID? You don't need it for Vgate iCar Pro BLE 4.0 adapter

  Run Android BLE scanner
  - choose IOS-VLINK device
  - get mac address a replace HM_MAC constant, then open CUSTOM service (first of 2)
  - there is serviceUUID (replace bellow in code)
  - open it.. there are 2x custom characteristics (first is for NOTIFY (read), and second for WRITE,WRITE_REQUEST).
  set charTxUUID with UUID from NOTIFY section
  set charRxUUID with UUID from WRITE section

  ---
  eNiro/Kona chargins limits depending on battery temperature (min.value of 01-04 battery module)
  >= 35°C BMS allows max 180A
  >= 25°C without limit (200A)
  >= 15°C BMS allows max 120A
  >= 5°C BMS allows max 90A
  >= 1°C BMS allows max 60A
  <= 0°C BMS allows max 40A
*/

#include "SPI.h"
#include "TFT_eSPI.h"
#include "BLEDevice.h"
#include <EEPROM.h>
#include <sys/time.h>
#include "struct.h"
#include "car_kia_eniro.h"
#include "car_hyundai_ioniq.h"
#include "car_debug_obd2_kia.h"

// PLEASE CHANGE THIS SETTING for your BLE4
uint32_t PIN = 1234;

// LILYGO TTGO T4 v1.3 BUTTONS
#define BUTTON_MIDDLE 37
#define BUTTON_LEFT 38
#define BUTTON_RIGHT 39
#define TFT_BL 4

/* TFT COLORS */
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F      
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */

#define TFT_DEFAULT_BK     0x0000    // 0x38E0
#define TFT_TEMP        0x0000    // NAVY
#define TFT_GREENYELLOW 0xB7E0
#define TFT_DARKRED     0x3800      /* 128,   0,   0 */
#define TFT_DARKGREEN2  0x01E0      /* 128,   0,   0 */

// Misc
#define GFXFF 1  // TFT FOnts

TFT_eSPI tft = TFT_eSPI();

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
#define displayScreenCount 6
byte displayScreen  = 1; // 0 - blank screen, 1 - automatic mode, 2 - dash board (default), 3 - big speed + kwh/100, 4 - battery cells, 5 - charging graph, 6 - soc10% CED table
byte displayScreenAutoMode = 0;
bool btnLeftPressed   = true;
bool btnMiddlePressed = true;
bool btnRightPressed  = true;

// Menu id/parent/title
typedef struct {
  int16_t id;
  int16_t parentId;
  int16_t targetParentId;
  char title[50];
  char obdMacAddress[20];
  char serviceUUID[40];
} MENU_ITEM;

#define menuItemsCount 43
bool menuVisible = false;
uint16_t menuCurrent = 0;
uint8_t  menuItemSelected = 0;
uint16_t scanningDeviceIndex = 0;
MENU_ITEM menuItems[menuItemsCount] = {

  {0, 0, 0, "<- exit menu"},
  {1, 0, -1, "Vehicle type"},
  {2, 0, -1, "Select OBD2BLE adapter"},
  {3, 0, -1, "Others"},
  {4, 0, -1, "Units"},
  {8, 0, -1, "Factory reset"},
  {9, 0, -1, "Save settings"},

  {100, 1, 0, "<- parent menu"},
  {101, 1, -1,  "Kia eNiro 2020 64kWh"},
  {102, 1, -1,  "Hyundai Kona 2020 64kWh"},
  {103, 1, -1,  "Hyundai Ioniq 2018 28kWh"},
  {104, 1, -1,  "Kia eNiro 2020 39kWh"},
  {105, 1, -1,  "Hyundai Kona 2020 39kWh"},
  {106, 1, -1,  "Debug OBD2 Kia"},

  {300, 3, 0, "<- parent menu"},
  {301, 3, -1, "Screen rotation"},

  {400, 4, 0, "<- parent menu"},
  {401, 4, -1, "Distance"},
  {402, 4, -1, "Temperature"},
  {403, 4, -1, "Pressure"},

  {3010, 301, 3, "<- parent menu"},
  {3011, 301, -1, "Normal"},
  {3012, 301, -1, "Flip vertical"},

  {4010, 401, 4, "<- parent menu"},
  {4011, 401, -1, "Kilometers"},
  {4012, 401, -1, "Miles"},

  {4020, 402, 4, "<- parent menu"},
  {4021, 402, -1, "Celsius"},
  {4022, 402, -1, "Fahrenheit"},

  {4030, 403, 4, "<- parent menu"},
  {4031, 403, -1, "Bar"},
  {4032, 403, -1, "Psi"},

  {9999, 9998, 0, "List of BLE devices"},
  {10000, 9999, 0, "<- parent menu"},
  {10001, 9999, -1, "-"},
  {10002, 9999, -1, "-"},
  {10003, 9999, -1, "-"},
  {10004, 9999, -1, "-"},
  {10005, 9999, -1, "-"},
  {10006, 9999, -1, "-"},
  {10007, 9999, -1, "-"},
  {10008, 9999, -1, "-"},
  {10009, 9999, -1, "-"},
};

/**
  Load settings from flash memory, upgrade structure if version differs
*/
bool saveSettings() {

  // Flash to memory
  Serial.println("Settings saved to eeprom.");
  EEPROM.put(0, settings);
  EEPROM.commit();

  return true;
}

/**
  Reset settings (factory reset)
*/
bool resetSettings() {

  // Flash to memory
  Serial.println("Factory reset.");
  settings.initFlag = 1;
  EEPROM.put(0, settings);
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
  settings.initFlag = 183;
  settings.settingsVersion = 1;
  settings.carType = CAR_KIA_ENIRO_2020_64;

  // Default OBD adapter MAC and UUID's
  tmpStr = "00:00:00:00:00:00"; // Pair via menu (middle button)
  tmpStr.toCharArray(settings.obdMacAddress, 18);
  tmpStr = "000018f0-0000-1000-8000-00805f9b34fb"; // Default UUID's for VGate iCar Pro BLE4 adapter
  tmpStr.toCharArray(settings.serviceUUID, 37);
  tmpStr = "00002af0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(settings.charTxUUID, 37);
  tmpStr = "00002af1-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(settings.charRxUUID, 37);

  settings.displayRotation = 1; // 1,3
  settings.distanceUnit = 'k';
  settings.temperatureUnit = 'c';
  settings.pressureUnit = 'b';

  // Load settings and replace default values
  Serial.println("Reading settings from eeprom.");
  EEPROM.begin(sizeof(SETTINGS_STRUC));
  EEPROM.get(0, tmpSettings);

  // Init flash with default settings
  if (tmpSettings.initFlag != 183) {
    Serial.println("Settings not found. Initialization.");
    saveSettings();
  } else {
    Serial.print("Loaded settings ver.: ");
    Serial.println(settings.settingsVersion);
    // Save version? No need to upgrade structure
    if (settings.settingsVersion == tmpSettings.settingsVersion) {
      settings = tmpSettings;
    }
  }

  // Load command queue
  if (settings.carType == CAR_KIA_ENIRO_2020_64 || settings.carType == CAR_HYUNDAI_KONA_2020_64 ||
      settings.carType == CAR_KIA_ENIRO_2020_39 || settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    activateCommandQueueForKiaENiro();
  }
  if (settings.carType == CAR_HYUNDAI_IONIQ_2018) {
    activateCommandQueueForHyundaiIoniq();
  }
  if (settings.carType == CAR_DEBUG_OBD2_KIA) {
    activateCommandQueueForDebugObd2Kia();
  }

  return true;
}

/**
   Init structure with data
*/
bool initStructure() {

  params.chargingStartTime = params.currentTime = 0;
  params.lightInfo = 0;
  params.brakeLightInfo = 0;
  params.driveMode = 0;
  params.espState = 0;
  params.speedKmh = -1;
  params.odoKm = -1;
  params.socPerc = -1;
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

  oldParams = params;

  return true;
}

/**
   Convert km to km or miles
*/
float km2distance(float inKm) {
  return (settings.distanceUnit == 'k') ? inKm : inKm / 1.609344;
}

/**
   Convert celsius to celsius or farenheit
*/
float celsius2temperature(float inCelsius) {
  return (settings.temperatureUnit == 'c') ? inCelsius : (inCelsius * 1.8) + 32;
}

/**
   Convert bar to bar or psi
*/
float bar2pressure(float inBar) {
  return (settings.pressureUnit == 'b') ? inBar : inBar * 14.503773800722;
}

/**
   Clear screen a display two lines message
*/
bool displayMessage(const char* row1, const char* row2) {

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
  Draw cell on dashboard
*/
bool monitoringRect(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, int16_t bgColor, int16_t fgColor) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  tft.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  bgColor);
  tft.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  tft.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
  tft.setTextDatum(TL_DATUM); // Topleft
  tft.setTextColor(TFT_SILVER, bgColor); // Bk, fg color
  tft.setTextSize(1); // Size for small 5x7 font
  tft.drawString(desc, posx, posy, 2);

  // Big 2x2 cell in the middle of screen
  if (w == 2 && h == 2) {

    // Bottom 2 numbers with charged/discharged kWh from start
    posx = (x * 80) + 5;
    posy = ((y + h) * 60) - 32;
    sprintf(tmpStr3, "-%01.01f", params.cumulativeEnergyDischargedKWh - params.cumulativeEnergyDischargedKWhStart);
    tft.setFreeFont(&Roboto_Thin_24);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(tmpStr3, posx, posy, GFXFF);

    posx = ((x + w) * 80) - 8;
    sprintf(tmpStr3, "+%01.01f", params.cumulativeEnergyChargedKWh - params.cumulativeEnergyChargedKWhStart);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(tmpStr3, posx, posy, GFXFF);

    // Main number - kwh on roads, amps on charges
    posy = (y * 60) + 24;
    tft.setTextColor(fgColor, bgColor);
    tft.setFreeFont(&Orbitron_Light_32);
    tft.drawString(text, posx, posy, 7);

  } else {

    // All others 1x1 cells
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(fgColor, bgColor);
    tft.setFreeFont(&Orbitron_Light_24);
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    tft.drawString(text, posx, posy, (w == 2 ? 7 : GFXFF));
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

  tft.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
  tft.drawFastVLine(((x + w) * 80) - 1, ((y) * 32) - 1, h * 32, TFT_BLACK);
  tft.drawFastHLine(((x) * 80) - 1, ((y + h) * 32) - 1, w * 80, TFT_BLACK);
  tft.setTextDatum(TL_DATUM); // Topleft
  tft.setTextColor(TFT_SILVER, bgColor); // Bk, fg bgColor
  tft.setTextSize(1); // Size for small 5x7 font
  tft.drawString(desc, posx, posy, 2);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(fgColor, bgColor);
  posx = (x * 80) + (w * 80 / 2) - 3;
  tft.drawString(text, posx, posy + 14, 2);

  return true;
}

/**
  Show tire pressures / temperatures
  Custom field
*/
bool showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char* topleft, const char* topright, const char* bottomleft, const char* bottomright, int16_t color) {

  int32_t posx, posy;

  tft.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  color);
  tft.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  tft.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_SILVER, color);
  tft.setTextSize(1);
  posx = (x * 80) + 4;
  posy = (y * 60) + 0;
  tft.drawString(topleft, posx, posy, 2);
  posy = (y * 60) + 14;
  tft.drawString(bottomleft, posx, posy, 2);

  tft.setTextDatum(TR_DATUM);
  posx = ((x + w) * 80) - 4;
  posy = (y * 60) + 0;
  tft.drawString(topright, posx, posy, 2);
  posy = (y * 60) + 14;
  tft.drawString(bottomright, posx, posy, 2);

  return true;
}

/**
   Main screen (Screen 1)
*/
bool drawSceneMain(bool force) {

  // Tire pressure
  if (force || params.tireFrontLeftTempC != oldParams.tireFrontLeftTempC
      || params.tireFrontRightTempC != oldParams.tireFrontRightTempC || params.tireRearLeftTempC != oldParams.tireRearLeftTempC || params.tireRearRightTempC != oldParams.tireRearRightTempC
      || oldParams.cumulativeEnergyChargedKWhStart != params.cumulativeEnergyChargedKWhStart
      || oldParams.cumulativeEnergyChargedKWh != params.cumulativeEnergyChargedKWh
      || oldParams.cumulativeEnergyDischargedKWhStart != params.cumulativeEnergyDischargedKWhStart
      || oldParams.cumulativeEnergyDischargedKWh != params.cumulativeEnergyDischargedKWh
     ) {
    char pressureStr[4] = "bar";
    char temperatureStr[2] = "C";
    if (settings.pressureUnit != 'b')
      strcpy(pressureStr, "psi");
    if (settings.temperatureUnit != 'c')
      strcpy(temperatureStr, "F");
    sprintf(tmpStr1, "%01.01f%s %02.00f%s", bar2pressure(params.tireFrontLeftPressureBar), pressureStr, celsius2temperature(params.tireFrontLeftTempC), temperatureStr);
    sprintf(tmpStr2, "%02.00f%s %01.01f%s", celsius2temperature(params.tireFrontRightTempC), temperatureStr, bar2pressure(params.tireFrontRightPressureBar), pressureStr);
    sprintf(tmpStr3, "%01.01f%s %02.00f%s", bar2pressure(params.tireRearLeftPressureBar), pressureStr, celsius2temperature(params.tireRearLeftTempC), temperatureStr);
    sprintf(tmpStr4, "%02.00f%s %01.01f%s", celsius2temperature(params.tireRearRightTempC), temperatureStr, bar2pressure(params.tireRearRightPressureBar), pressureStr);
    showTires(1, 0, 2, 1, tmpStr1, tmpStr2, tmpStr3, tmpStr4, TFT_BLACK);

    // Added later - kwh total in tires box
    // TODO: refactoring
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    sprintf(tmpStr1, "C: %01.01f +%01.01fkWh", params.cumulativeEnergyChargedKWh, params.cumulativeEnergyChargedKWh - params.cumulativeEnergyChargedKWhStart);
    tft.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 30, 2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    sprintf(tmpStr1, "D: %01.01f -%01.01fkWh", params.cumulativeEnergyDischargedKWh, params.cumulativeEnergyDischargedKWh - params.cumulativeEnergyDischargedKWhStart);
    tft.drawString(tmpStr1, (1 * 80) + 4, (0 * 60) + 44, 2);

    oldParams.tireFrontLeftTempC = params.tireFrontLeftTempC;
    oldParams.tireFrontLeftPressureBar = params.tireFrontLeftPressureBar;
    oldParams.tireFrontRightTempC = params.tireFrontRightTempC;
    oldParams.tireFrontRightPressureBar = params.tireFrontRightPressureBar;
    oldParams.tireRearLeftTempC = params.tireRearLeftTempC;
    oldParams.tireRearLeftPressureBar = params.tireRearLeftPressureBar;
    oldParams.tireRearRightTempC = params.tireRearRightTempC;
    oldParams.tireRearRightPressureBar = params.tireRearRightPressureBar;
    oldParams.cumulativeEnergyChargedKWhStart = params.cumulativeEnergyChargedKWhStart;
    oldParams.cumulativeEnergyChargedKWh = params.cumulativeEnergyChargedKWh;
    oldParams.cumulativeEnergyDischargedKWhStart = params.cumulativeEnergyDischargedKWhStart;
    oldParams.cumulativeEnergyDischargedKWh = params.cumulativeEnergyDischargedKWh;
  }

  // batPowerKwh100 on roads, else batPowerAmp
  if (params.speedKmh > 20) {
    if (force || params.batPowerKwh100 != oldParams.batPowerKwh100) {
      sprintf(tmpStr1, "%01.01f", km2distance(params.batPowerKwh100));
      monitoringRect(1, 1, 2, 2, tmpStr1, ((settings.distanceUnit == 'k') ? "POWER KWH/100KM" : "POWER KWH/100MI"), (params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
      oldParams.batPowerKwh100 = params.batPowerKwh100;
    }
  } else {
    // batPowerAmp on chargers (under 10kmh)
    if (force || params.batPowerKw != oldParams.batPowerKw) {
      sprintf(tmpStr1, "%01.01f", params.batPowerKw);
      monitoringRect(1, 1, 2, 2, tmpStr1, "POWER KW", (params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
      oldParams.batPowerKw = params.batPowerKw;
    }
  }

  // socPerc
  if (force || params.socPerc != oldParams.socPerc) {
    sprintf(tmpStr1, "%01.00f%%", params.socPerc);
    sprintf(tmpStr2, (params.sohPerc ==  100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), params.sohPerc);
    monitoringRect(0, 0, 1, 1, tmpStr1, tmpStr2, (params.socPerc < 10 || params.sohPerc < 100 ? TFT_RED : (params.socPerc  > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)), TFT_WHITE);
    oldParams.socPerc = params.socPerc;
    oldParams.sohPerc = params.sohPerc;
  }

  // batPowerAmp
  if (force || params.batPowerAmp != oldParams.batPowerAmp) {
    sprintf(tmpStr1, (abs(params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), params.batPowerAmp);
    monitoringRect(0, 1, 1, 1, tmpStr1, "CURRENT A", (params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);
    oldParams.batPowerAmp = params.batPowerAmp;
  }

  // batVoltage
  if (force || params.batVoltage != oldParams.batVoltage) {
    sprintf(tmpStr1, "%03.00f", params.batVoltage);
    monitoringRect(0, 2, 1, 1, tmpStr1, "VOLTAGE", TFT_DEFAULT_BK, TFT_WHITE);
    oldParams.batVoltage = params.batVoltage;
  }

  // batCellMinV
  if (force || params.batCellMinV != oldParams.batCellMinV || params.batCellMaxV != oldParams.batCellMaxV) {
    sprintf(tmpStr1, "%01.02f", params.batCellMaxV - params.batCellMinV);
    sprintf(tmpStr2, "CELLS %01.02f", params.batCellMinV);
    monitoringRect(0, 3, 1, 1, ( params.batCellMaxV - params.batCellMinV == 0.00 ? "OK" : tmpStr1), tmpStr2, TFT_DEFAULT_BK, TFT_WHITE);
    oldParams.batCellMaxV = params.batCellMaxV;
    oldParams.batCellMinV = params.batCellMinV;
  }

  // batTempC
  if (force || params.batTempC != oldParams.batTempC) {
    sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), celsius2temperature(params.batMinC));
    sprintf(tmpStr2, ((settings.temperatureUnit == 'c') ? "BATT. %01.00fC" : "BATT. %01.01fF"), celsius2temperature(params.batMaxC));
    monitoringRect(1, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, (params.batTempC >= 15) ? ((params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    oldParams.batTempC = params.batTempC;
  }

  // batHeaterC
  if (force || params.batHeaterC != oldParams.batHeaterC) {
    sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f" : "%01.01f"), celsius2temperature(params.batHeaterC));
    monitoringRect(2, 3, 1, 1, tmpStr1, "BAT.HEAT", TFT_TEMP, TFT_WHITE);
    oldParams.batHeaterC = params.batHeaterC;
  }

  // Aux perc
  if (force || params.auxPerc != oldParams.auxPerc) {
    sprintf(tmpStr1, "%01.00f%%", params.auxPerc);
    monitoringRect(3, 0, 1, 1, tmpStr1, "AUX BAT.", (params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK), TFT_WHITE);
    oldParams.auxPerc = params.auxPerc;
  }

  // Aux amp
  if (force || params.auxCurrentAmp != oldParams.auxCurrentAmp) {
    sprintf(tmpStr1, (abs(params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), params.auxCurrentAmp);
    monitoringRect(3, 1, 1, 1, tmpStr1, "AUX AMPS",  (params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);
    oldParams.auxCurrentAmp = params.auxCurrentAmp;
  }

  // auxVoltage
  if (force || params.auxVoltage != oldParams.auxVoltage) {
    sprintf(tmpStr1, "%01.01f", params.auxVoltage);
    monitoringRect(3, 2, 1, 1, tmpStr1, "AUX VOLTS", (params.auxVoltage < 12.1 ? TFT_RED : (params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)), TFT_WHITE);
    oldParams.auxVoltage = params.auxVoltage;
  }

  // indoorTemperature
  if (force || params.indoorTemperature != oldParams.indoorTemperature || params.outdoorTemperature != oldParams.outdoorTemperature) {
    sprintf(tmpStr1, "%01.01f", celsius2temperature(params.indoorTemperature));
    sprintf(tmpStr2, "IN/OUT%01.01f", celsius2temperature(params.outdoorTemperature));
    monitoringRect(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, TFT_WHITE);
    oldParams.indoorTemperature = params.indoorTemperature;
    oldParams.outdoorTemperature = params.outdoorTemperature;
  }

  return true;
}

/**
   Speed + kwh/100km (Screen 2)
*/
bool drawSceneSpeed(bool force) {

  int32_t posx, posy;

  tft.fillRect(0, 36, 200, 160, TFT_DARKRED);

  posx = 320 / 2;
  posy = 40;
  tft.setTextDatum(TR_DATUM); // Top center
  tft.setTextColor(TFT_WHITE, TFT_DARKRED);
  tft.setTextSize(2); // Size for small 5cix7 font
  sprintf(tmpStr3, "0");
  if (params.speedKmh > 10)
    sprintf(tmpStr3, "%01.00f", km2distance(params.speedKmh));
  tft.drawString(tmpStr3, 200, posy, 7);

  posy = 145;
  tft.setTextDatum(TR_DATUM); // Top center
  tft.setTextSize(1);
  if (params.speedKmh > 25 && params.batPowerKw < 0) {
    sprintf(tmpStr3, "%01.01f", km2distance(params.batPowerKwh100));
  } else {
    sprintf(tmpStr3, "%01.01f", params.batPowerKw);
  }
  tft.drawString(tmpStr3, 200, posy, 7);

  // Bottom 2 numbers with charged/discharged kWh from start
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  posx = 5;
  posy = 5;
  sprintf(tmpStr3, ((settings.distanceUnit == 'k') ? "%01.00fkm  " : "%01.00fmi  "), km2distance(params.odoKm));
  tft.setTextDatum(TL_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);

  // Bottom info
  // Cummulative regen/power
  posy = 240 - 5;
  sprintf(tmpStr3, "-%01.01f    ", params.cumulativeEnergyDischargedKWh - params.cumulativeEnergyDischargedKWhStart);
  tft.setTextDatum(BL_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);
  posx = 320 - 5;
  sprintf(tmpStr3, "    +%01.01f", params.cumulativeEnergyChargedKWh - params.cumulativeEnergyChargedKWhStart);
  tft.setTextDatum(BR_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);
  // Bat.power
  posx = 320 / 2;
  sprintf(tmpStr3, "   %01.01fkw   ", params.batPowerKw);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);

  // RIGHT INFO
  // Battery "cold gate" detection - red < 15C (43KW limit), <25 (blue - 55kW limit), green all ok
  tft.fillCircle(290, 60, 25, (params.batTempC >= 15) ? ((params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  tft.setTextColor(TFT_WHITE, (params.batTempC >= 15) ? ((params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(MC_DATUM);
  sprintf(tmpStr3, "%01.00f", celsius2temperature(params.batTempC));
  tft.drawString(tmpStr3, 290, 60, GFXFF);

  tft.setTextDatum(MR_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  sprintf(tmpStr3, "  %d", params.brakeLightInfo);
  tft.drawString(tmpStr3, 250, 20, GFXFF);
  sprintf(tmpStr3, "  %d", params.lightInfo);
  tft.drawString(tmpStr3, 250, 50, GFXFF);
  sprintf(tmpStr3, "  %d", params.driveMode);
  tft.drawString(tmpStr3, 250, 80, GFXFF);
  
  // Soc%, bat.kWh
  tft.setFreeFont(&Orbitron_Light_32);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  sprintf(tmpStr3, " %01.00f%%", params.socPerc);
  tft.drawString(tmpStr3, 320, 94, GFXFF);
  if (params.socPerc > 0) {
    sprintf(tmpStr3, " %01.01f", params.batteryTotalAvailableKWh * (params.socPerc / 100));
    tft.drawString(tmpStr3, 320, 129, GFXFF);
    tft.drawString(" kWh", 320, 164, GFXFF);
  }

  return true;
}

/**
   Battery cells (Screen 3)
*/
bool drawSceneBatteryCells(bool force) {

  int32_t posx, posy;

  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batHeaterC));
  drawSmallRect(0, 0, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batInletC));
  drawSmallRect(1, 0, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batModuleTempC[0]));
  drawSmallRect(0, 1, 1, 1, tmpStr1, "MO1", TFT_TEMP, (params.batModuleTempC[0] >= 15) ? ((params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batModuleTempC[1]));
  drawSmallRect(1, 1, 1, 1, tmpStr1, "MO2", TFT_TEMP, (params.batModuleTempC[1] >= 15) ? ((params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batModuleTempC[2]));
  drawSmallRect(2, 1, 1, 1, tmpStr1, "MO3", TFT_TEMP, (params.batModuleTempC[2] >= 15) ? ((params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batModuleTempC[3]));
  drawSmallRect(3, 1, 1, 1, tmpStr1, "MO4", TFT_TEMP, (params.batModuleTempC[3] >= 15) ? ((params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  // Ioniq (up to 12 cells)
  for (uint16_t i = 4; i < params.batModuleTempCount; i++) {
    if (params.batModuleTempC[i] == 0)
      continue;
    posx = (((i - 4) % 8) * 40);
    posy = ((floor((i - 4) / 8)) * 13) + 64;
    //tft.fillRect(x * 80, y * 32, ((w) * 80), ((h) * 32),  bgColor);
    tft.setTextSize(1); // Size for small 5x7 font
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(((params.batModuleTempC[i] >= 15) ? ((params.batModuleTempC[i] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED), TFT_BLACK);
    sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00fC" : "%01.01fF"), celsius2temperature(params.batModuleTempC[i]));
    tft.drawString(tmpStr1, posx + 4, posy, 2);
  }

  tft.setTextDatum(TL_DATUM); // Topleft
  tft.setTextSize(1); // Size for small 5x7 font

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < 98; i++) {
    if ((params.cellVoltage[i] < minVal || minVal == -1) && params.cellVoltage[i] != -1)
      minVal = params.cellVoltage[i];
    if ((params.cellVoltage[i] > maxVal || maxVal == -1) && params.cellVoltage[i] != -1)
      maxVal = params.cellVoltage[i];
    if (params.cellVoltage[i] > 0 && i > params.cellCount + 1)
      params.cellCount = i + 1;
  }

  // Draw cell matrix
  for (int i = 0; i < 98; i++) {
    if (params.cellVoltage[i] == -1)
      continue;
    posx = ((i % 8) * 40) + 4;
    posy = ((floor(i / 8) + (params.cellCount > 96 ? 0 : 1)) * 13) + 68;
    sprintf(tmpStr3, "%01.02f", params.cellVoltage[i]);
    tft.setTextColor(TFT_NAVY, TFT_BLACK);
    if (params.cellVoltage[i] == minVal && minVal != maxVal)
      tft.setTextColor(TFT_RED, TFT_BLACK);
    if (params.cellVoltage[i] == maxVal && minVal != maxVal)
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(tmpStr3, posx, posy, 2);
  }

  return true;
}

/**
   Charging graph (Screen 4)
*/
bool drawSceneChargingGraph(bool force) {

  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3; // 100% = 300px;
  int mulY = 2; // 100kW = 200px
  int maxKw = 80;
  int posy = 0;
  int16_t color;

  sprintf(tmpStr1, "%01.00f", params.socPerc);
  drawSmallRect(0, 0, 1, 1, tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", params.batPowerKw);
  drawSmallRect(1, 0, 1, 1, tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", params.batPowerAmp);
  drawSmallRect(2, 0, 1, 1, tmpStr1, "CURRENT A", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%03.00f", params.batVoltage);
  drawSmallRect(3, 0, 1, 1, tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batHeaterC));
  drawSmallRect(0, 1, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_RED);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batInletC));
  drawSmallRect(1, 1, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.batMinC));
  drawSmallRect(2, 1, 1, 1, tmpStr1, "BAT.MIN", (params.batMinC >= 15) ? ((params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "%01.00f C" : "%01.01f F"), celsius2temperature(params.outdoorTemperature));
  drawSmallRect(3, 1, 1, 1, tmpStr1, "OUT.TEMP.", TFT_TEMP, TFT_CYAN);

  tft.setTextColor(TFT_SILVER, TFT_TEMP);

  for (int i = 0; i <= 10; i++) {
    color = TFT_DARKRED;
    if (i == 0 || i == 5 || i == 10)
      color = TFT_NAVY;
    tft.drawFastVLine(zeroX + (i * 10 * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
    /*if (i != 0 && i != 10) {
      sprintf(tmpStr1, "%d%%", i * 10);
      tft.setTextDatum(BC_DATUM);
      tft.drawString(tmpStr1, zeroX + (i * 10 * mulX),  zeroY - (maxKw * mulY), 2);
      }*/
    if (i <= (maxKw / 10)) {
      tft.drawFastHLine(zeroX, zeroY - (i * 10 * mulY), 100 * mulX, color);
      if (i > 0) {
        sprintf(tmpStr1, "%d", i * 10);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(tmpStr1, zeroX + (100 * mulX) + 3, zeroY - (i * 10 * mulY), 2);
      }
    }
  }

  for (int i = 0; i <= 100; i++) {
    if (params.chargingGraphBatMinTempC[i] > -10)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphBatMinTempC[i]*mulY), mulX, TFT_BLUE);
    if (params.chargingGraphBatMaxTempC[i] > -10)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphBatMaxTempC[i]*mulY), mulX, TFT_BLUE);
    if (params.chargingGraphWaterCoolantTempC[i] > -10)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphWaterCoolantTempC[i]*mulY), mulX, TFT_PURPLE);
    if (params.chargingGraphHeaterTempC[i] > -10)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphHeaterTempC[i]*mulY), mulX, TFT_RED);

    if (params.chargingGraphMinKw[i] > 0)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphMinKw[i]*mulY), mulX, TFT_GREENYELLOW);
    if (params.chargingGraphMaxKw[i] > 0)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphMaxKw[i]*mulY), mulX, TFT_YELLOW);
  }

  // Bat.module temperatures
  tft.setTextSize(1); // Size for small 5x7 font
  tft.setTextDatum(BL_DATUM);
  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "1=%01.00fC " : "1=%01.00fF "), celsius2temperature(params.batModuleTempC[0]));
  tft.setTextColor((params.batModuleTempC[0] >= 15) ? ((params.batModuleTempC[0] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, 0,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "2=%01.00fC " : "2=%01.00fF "), celsius2temperature(params.batModuleTempC[1]));
  tft.setTextColor((params.batModuleTempC[1] >= 15) ? ((params.batModuleTempC[1] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, 48,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "3=%01.00fC " : "3=%01.00fF "), celsius2temperature(params.batModuleTempC[2]));
  tft.setTextColor((params.batModuleTempC[2] >= 15) ? ((params.batModuleTempC[2] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, 96,  zeroY - (maxKw * mulY), 2);

  sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "4=%01.00fC " : "4=%01.00fF "), celsius2temperature(params.batModuleTempC[3]));
  tft.setTextColor((params.batModuleTempC[3] >= 15) ? ((params.batModuleTempC[3] >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, 144,  zeroY - (maxKw * mulY), 2);
  sprintf(tmpStr1, "ir %01.00fkOhm", params.isolationResistanceKOhm );

  // Bms max.regen/power available
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(tmpStr1, "xC=%01.00fkW ", params.availableChargePower);
  tft.drawString(tmpStr1, 192,  zeroY - (maxKw * mulY), 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(tmpStr1, "xD=%01.00fkW", params.availableDischargePower);
  tft.drawString(tmpStr1, 256,  zeroY - (maxKw * mulY), 2);

  //
  tft.setTextDatum(TR_DATUM);
  if (params.coolingWaterTempC != -1) {
    sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "W=%01.00fC" : "W=%01.00fF"), celsius2temperature(params.coolingWaterTempC));
    tft.setTextColor(TFT_PURPLE, TFT_TEMP);
    tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (params.batFanFeedbackHz != -1) {
    sprintf(tmpStr1, "FF=%03.00fHz", params.batFanFeedbackHz);
    tft.setTextColor(TFT_WHITE, TFT_TEMP);
    tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (params.batFanStatus != -1) {
    sprintf(tmpStr1, "FS=%03.00f", params.batFanStatus);
    tft.setTextColor(TFT_WHITE, TFT_TEMP);
    tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (params.coolantTemp1C != -1) {
    sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "C1:%01.00fC" : "C1:%01.00fF"), celsius2temperature(params.coolantTemp1C));
    tft.setTextColor(TFT_WHITE, TFT_TEMP);
    tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }
  if (params.coolantTemp2C != -1) {
    sprintf(tmpStr1, ((settings.temperatureUnit == 'c') ? "C2:%01.00fC" : "C2:%01.00fF"), celsius2temperature(params.coolantTemp2C));
    tft.setTextColor(TFT_WHITE, TFT_TEMP);
    tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + (posy * 15), 2);
    posy++;
  }

  // Print charging time
  time_t diffTime = params.currentTime - params.chargingStartTime;
  if ((diffTime / 60) > 99)
    sprintf(tmpStr1, "%02d:%02d:%02d", (diffTime / 3600) % 24, (diffTime / 60) % 60, diffTime % 60);
  else
    sprintf(tmpStr1, "%02d:%02d", (diffTime / 60), diffTime % 60);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_SILVER, TFT_BLACK);
  tft.drawString(tmpStr1, 160 - 10, 225, 2);

  return true;
}

/**
   SOC 10% table (screen 5)
*/
bool drawSceneSoc10Table(bool force) {

  int zeroY = 2;
  float diffCec, diffCed, diffOdo = -1;
  float firstCed = -1, lastCed = -1, diffCed0to5 = 0;
  float firstCec = -1, lastCec = -1, diffCec0to5 = 0;
  float firstOdo = -1, lastOdo = -1, diffOdo0to5 = 0;
  float diffTime;
  tft.setTextSize(1); // Size for small 5x7 font
  tft.setTextColor(TFT_SILVER, TFT_TEMP);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("CONSUMPTION | DISCH.100%->4% SOC",  2, zeroY, 2);

  tft.setTextDatum(TR_DATUM);

  tft.drawString("dis./char.kWh", 128, zeroY + (1 * 15), 2);
  tft.drawString(((settings.distanceUnit == 'k') ? "km" : "mi"), 160, zeroY + (1 * 15), 2);
  tft.drawString("kWh100", 224, zeroY + (1 * 15), 2);
  tft.drawString("avg.speed", 310, zeroY + (1 * 15), 2);

  for (int i = 0; i <= 10; i++) {
    sprintf(tmpStr1, "%d%%", (i == 0) ? 5 : i * 10);
    tft.drawString(tmpStr1, 32, zeroY + ((12 - i) * 15), 2);

    firstCed = (params.soc10ced[i] != -1) ? params.soc10ced[i] : firstCed;
    lastCed = (lastCed == -1 && params.soc10ced[i] != -1) ? params.soc10ced[i] : lastCed;
    firstCec = (params.soc10cec[i] != -1) ? params.soc10cec[i] : firstCec;
    lastCec = (lastCec == -1 && params.soc10cec[i] != -1) ? params.soc10cec[i] : lastCec;
    firstOdo = (params.soc10odo[i] != -1) ? params.soc10odo[i] : firstOdo;
    lastOdo = (lastOdo == -1 && params.soc10odo[i] != -1) ? params.soc10odo[i] : lastOdo;

    if (i != 10) {
      diffCec = (params.soc10cec[i + 1] != -1 && params.soc10cec[i] != -1) ? (params.soc10cec[i] - params.soc10cec[i + 1]) : 0;
      diffCed = (params.soc10ced[i + 1] != -1 && params.soc10ced[i] != -1) ? (params.soc10ced[i + 1] - params.soc10ced[i]) : 0;
      diffOdo = (params.soc10odo[i + 1] != -1 && params.soc10odo[i] != -1) ? (params.soc10odo[i] - params.soc10odo[i + 1]) : -1;
      diffTime = (params.soc10time[i + 1] != -1 && params.soc10time[i] != -1) ? (params.soc10time[i] - params.soc10time[i + 1]) : -1;
      if (diffCec != 0) {
        sprintf(tmpStr1, "+%01.01f", diffCec);
        tft.drawString(tmpStr1, 128, zeroY + ((12 - i) * 15), 2);
        diffCec0to5 = (i == 0) ? diffCec : diffCec0to5;
      }
      if (diffCed != 0) {
        sprintf(tmpStr1, "%01.01f", diffCed);
        tft.drawString(tmpStr1, 80, zeroY + ((12 - i) * 15), 2);
        diffCed0to5 = (i == 0) ? diffCed : diffCed0to5;
      }
      if (diffOdo != -1) {
        sprintf(tmpStr1, "%01.00f", km2distance(diffOdo));
        tft.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
        diffOdo0to5 = (i == 0) ? diffOdo : diffOdo0to5;
        if (diffTime > 0) {
          sprintf(tmpStr1, "%01.01f", km2distance(diffOdo) / (diffTime / 3600));
          tft.drawString(tmpStr1, 310, zeroY + ((12 - i) * 15), 2);
        }
      }
      if (diffOdo > 0 && diffCed != 0) {
        sprintf(tmpStr1, "%01.1f", (-diffCed * 100.0 / km2distance(diffOdo)));
        tft.drawString(tmpStr1, 224, zeroY + ((12 - i) * 15), 2);
      }
    }

    if (diffOdo == -1 && params.soc10odo[i] != -1) {
      sprintf(tmpStr1, "%01.00f", km2distance(params.soc10odo[i]));
      tft.drawString(tmpStr1, 160, zeroY + ((12 - i) * 15), 2);
    }
  }

  tft.drawString("0%", 32, zeroY + (13 * 15), 2);
  tft.drawString("0-5% is calculated (same) as 5-10%", 310, zeroY + (13 * 15), 2);

  tft.drawString("TOT.", 32, zeroY + (14 * 15), 2);
  diffCed = (lastCed != -1 && firstCed != -1) ? firstCed - lastCed + diffCed0to5 : 0;
  sprintf(tmpStr1, "%01.01f", diffCed);
  tft.drawString(tmpStr1, 80, zeroY + (14 * 15), 2);
  diffCec = (lastCec != -1 && firstCec != -1) ? lastCec - firstCec + diffCec0to5 : 0;
  sprintf(tmpStr1, "+%01.01f", diffCec);
  tft.drawString(tmpStr1, 128, zeroY + (14 * 15), 2);
  diffOdo = (lastOdo != -1 && firstOdo != -1) ? lastOdo - firstOdo + diffOdo0to5 : 0;
  sprintf(tmpStr1, "%01.00f", km2distance(diffOdo));
  tft.drawString(tmpStr1, 160, zeroY + (14 * 15), 2);
  sprintf(tmpStr1, "AVAIL.CAP: %01.01f kWh", -diffCed - diffCec);
  tft.drawString(tmpStr1, 310, zeroY + (14 * 15), 2);

  return true;
}

/**
   Modify caption
*/
String menuItemCaption(int16_t menuItemId, String title) {

  String prefix = "", suffix = "";

  if (menuItemId == 401) // distance
    suffix = (settings.distanceUnit == 'k') ? "[km]" : "[mi]";
  if (menuItemId == 402) // temperature
    suffix = (settings.temperatureUnit == 'c') ? "[C]" : "[F]";
  if (menuItemId == 403) // pressure
    suffix = (settings.pressureUnit == 'b') ? "[bar]" : "[psi]";

  title = ((prefix == "") ? "" : prefix + " ") + title + ((suffix == "") ? "" : " " + suffix);
  return title;
}

/**
  Display menu
*/
bool showMenu() {

  uint16_t posY = 0, tmpCurrMenuItem = 0;

  menuVisible = true;
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&Roboto_Thin_24);

  for (uint16_t i = 0; i < menuItemsCount; ++i) {
    if (menuCurrent == menuItems[i].parentId) {
      tft.fillRect(0, posY, 320, tft.fontHeight() + 2, (menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
      tft.setTextColor((menuItemSelected == tmpCurrMenuItem) ? TFT_WHITE : TFT_WHITE, (menuItemSelected == tmpCurrMenuItem) ? TFT_DARKGREEN2 : TFT_BLACK);
      tft.drawString(menuItemCaption(menuItems[i].id, menuItems[i].title), 0, posY + 2, GFXFF);
      posY += tft.fontHeight();
      tmpCurrMenuItem++;
    }
  }

  return true;
}

/**
   Hide menu
*/
bool hideMenu() {

  menuVisible = false;
  menuCurrent = 0;
  menuItemSelected = 0;
  redrawScreen(true);

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
    menuItemSelected = (menuItemSelected > tmpCount) ? tmpCount : menuItemSelected + 1;
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
      redrawScreen(true);
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
      strlcpy((char*)settings.obdMacAddress, (char*)tmpMenuItem.obdMacAddress, 20);
      Serial.print("Selected adapter MAC address ");
      Serial.println(settings.obdMacAddress);
      saveSettings();
      ESP.restart();
    }
    // Other menus
    switch (tmpMenuItem.id) {
      // Set vehicle type
      case 101: settings.carType = CAR_KIA_ENIRO_2020_64; break;
      case 102: settings.carType = CAR_HYUNDAI_KONA_2020_64; break;
      case 103: settings.carType = CAR_HYUNDAI_IONIQ_2018; break;
      case 104: settings.carType = CAR_KIA_ENIRO_2020_39; break;
      case 105: settings.carType = CAR_HYUNDAI_KONA_2020_39; break;
      case 106: settings.carType = CAR_DEBUG_OBD2_KIA; break;
      // Screen orientation
      case 3011: settings.displayRotation = 1; tft.setRotation(settings.displayRotation); break;
      case 3012: settings.displayRotation = 3; tft.setRotation(settings.displayRotation); break;
      // Distance
      case 4011: settings.distanceUnit = 'k'; break;
      case 4012: settings.distanceUnit = 'm'; break;
      // Temperature
      case 4021: settings.temperatureUnit = 'c'; break;
      case 4022: settings.temperatureUnit = 'f'; break;
      // Pressure
      case 4031: settings.pressureUnit = 'b'; break;
      case 4032: settings.pressureUnit = 'p'; break;
      // Pair ble device
      case 2: startBleScan(); return false;
      // Reset settings
      case 8: resetSettings(); hideMenu(); return false;
      // Save settings
      case 9: saveSettings(); break;
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
bool redrawScreen(bool force) {

  if (menuVisible) {
    return false;
  }

  // Clear screen if needed
  if (force) {
    tft.fillScreen(TFT_BLACK);
  }

  // 1. Auto mode = >5kpm Screen 3 - speed, other wise basic Screen2 - Main screen, if charging then Screen 5 Graph
  if (displayScreen == 1) {
    if (params.speedKmh > 5) {
      if (displayScreenAutoMode != 3) {
        tft.fillScreen(TFT_BLACK);
        displayScreenAutoMode = 3;
      }
      drawSceneSpeed(force);
    } else if (params.batPowerKw > 1) {
      if (displayScreenAutoMode != 5) {
        tft.fillScreen(TFT_BLACK);
        displayScreenAutoMode = 5;
      }
      drawSceneChargingGraph(force);
    } else {
      if (displayScreenAutoMode != 2) {
        tft.fillScreen(TFT_BLACK);
        displayScreenAutoMode = 2;
      }
      drawSceneMain(force);
    }
  }
  // 2. Main screen
  if (displayScreen == 2) {
    drawSceneMain(force);
  }
  // 3. Big speed + kwh/100km
  if (displayScreen == 3) {
    drawSceneSpeed(force);
  }
  // 4. Battery cells
  if (displayScreen == 4) {
    drawSceneBatteryCells(force);
  }
  // 5. Charging graph
  if (displayScreen == 5) {
    drawSceneChargingGraph(force);
  }
  // 6. SOC10% table (CEC-CED)
  if (displayScreen == 6) {
    drawSceneSoc10Table(force);
  }

  // BLE not connected
  if (!bleConnected && bleConnect) {
    // Print message
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(" BLE4 OBDII not connected... ", 0, 240 / 2, 2);
    tft.drawString(" Press middle button to menu. ", 0, (240 / 2) + tft.fontHeight(), 2);
  }
  return true;
}

/**
   Do next AT command from queue
*/
bool doNextAtCommand() {

  // Restart loop with AT commands
  if (commandQueueIndex >= commandQueueCount) {
    commandQueueIndex = commandQueueLoopFrom;
    // Redraw only changed values
    redrawScreen(false);
  }

  // Send AT command to obd
  commandRequest = commandQueue[commandQueueIndex];
  if (commandRequest.startsWith("ATSH")) {
    currentAtshRequest = commandRequest;
  }

  Serial.print(">>> ");
  Serial.println(commandRequest);
  String tmpStr = commandRequest + "\r";
  pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  commandQueueIndex++;

  return true;
}

/**
  Parse result from OBD, create single line responseRowMerged
*/
bool parseRow() {

  // Simple 1 line responses
  Serial.print("");
  Serial.println(responseRow);

  // Merge 0:xxxx 1:yyyy 2:zzzz to single xxxxyyyyzzzz string
  if (responseRow.length() >= 2 && responseRow.charAt(1) == ':') {
    if (responseRow.charAt(0) == '0') {
      responseRowMerged = "";
    }
    responseRowMerged += responseRow.substring(2);
  }

  return true;
}

/**
  Parse merged row (after merge completed)
*/
bool parseRowMerged() {

  Serial.print("merged:");
  Serial.println(responseRowMerged);

  if (settings.carType == CAR_KIA_ENIRO_2020_64 || settings.carType == CAR_HYUNDAI_KONA_2020_64 ||
      settings.carType == CAR_KIA_ENIRO_2020_39 || settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    parseRowMergedKiaENiro();
  }
  if (settings.carType == CAR_HYUNDAI_IONIQ_2018) {
    parseRowMergedHyundaiIoniq();
  }
  if (settings.carType == CAR_DEBUG_OBD2_KIA) {
    parseRowMergedDebugObd2Kia();
  }

  return true;
}

/**
   Parse test data
*/
bool testData() {

  redrawScreen(true);

  if (settings.carType == CAR_KIA_ENIRO_2020_64 || settings.carType == CAR_HYUNDAI_KONA_2020_64 ||
      settings.carType == CAR_KIA_ENIRO_2020_39 || settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    testDataKiaENiro();
  }
  if (settings.carType == CAR_HYUNDAI_IONIQ_2018) {
    testDataHyundaiIoniq();
  }

  redrawScreen(false);
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

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(settings.serviceUUID)) &&
          (strcmp(advertisedDevice.getAddress().toString().c_str(), settings.obdMacAddress) == 0)) {
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
static void notifyCallback (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  char ch;

  // Parse multi line response to single lines
  responseRow = "";
  for (int i = 0; i <= length; i++) {
    ch = pData[i];
    if (ch == '\r'  || ch == '\n' || ch == '\0') {
      if (responseRow != "")
        parseRow();
      responseRow = "";
    } else {
      responseRow += ch;
      if (responseRow == ">") {
        if (responseRowMerged != "") {
          parseRowMerged();
        }
        responseRowMerged = "";
        canSendNextAtCommand = true;
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
  if ( pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) Serial.println("bleConnected");
  Serial.println(" - bleConnected to server");

  displayMessage(" > Connecting device", "Connecting service...");
  // Remote service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(settings.serviceUUID));
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(settings.serviceUUID);
    displayMessage(" > Connecting device", "Unable to find service");
    return false;
  }
  Serial.println(" - Found our service");

  // Get characteristics
  displayMessage(" > Connecting device", "Connecting TxUUID...");
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(settings.charTxUUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(settings.charTxUUID);//.toString().c_str());
    displayMessage(" > Connecting device", "Unable to find TxUUID");
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Get characteristics
  displayMessage(" > Connecting device", "Connecting RxUUID...");
  pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(BLEUUID(settings.charRxUUID));
  if (pRemoteCharacteristicWrite == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(settings.charRxUUID);//.toString().c_str());
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
  Serial.println(settings.obdMacAddress);
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
      redrawScreen(true);
    }
  }

  return true;
}

/**
  Setup device
*/
void setup(void) {

  // Serial console, init structures
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Booting device...");

  initStructure();
  loadSettings();

  // Set button pins for input
  pinMode(BUTTON_MIDDLE, INPUT);
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  // Init display
  Serial.println("Init TFT display");
  tft.begin();

  //  tft.invertDisplay(false);  // ONLY TTGO-TM
  tft.setRotation(settings.displayRotation);
  tft.fillScreen(TFT_BLACK);
  redrawScreen(true);

  // Init time library
  struct timeval tv;
  tv.tv_sec = 1589011873;
  settimeofday(&tv, NULL);

  struct tm now;
  getLocalTime(&now, 0);
  params.chargingStartTime = params.currentTime = mktime(&now);

  // Show test data on right button during boot device
  if (digitalRead(BUTTON_RIGHT) == LOW) {
    displayScreen = 1;
    testData();
  }

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

  // End
  Serial.println("Device setup completed");
}

/**
  Loop
*/
void loop() {

  //Serial.println("Loop");

  // Connect BLE device
  if (bleConnect == true && foundMyBleDevice != NULL) {
    pServerAddress = new BLEAddress(settings.obdMacAddress);
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

  // Send line from TTY to OBD (custom command)
  if (bleConnected) {
    if (Serial.available()) {
      ch = Serial.read();
      line = line + ch;
      if (ch == '\r' || ch == '\n') {
        Serial.print("Sending line: ");
        Serial.println(line);
        pRemoteCharacteristicWrite->writeValue(line.c_str(), line.length());
        line = "";
      }
    }

    // Can send next command from queue to OBD
    if (canSendNextAtCommand) {
      canSendNextAtCommand = false;
      doNextAtCommand();
    }
  }

  // Handle buttons (under construction) LOW - pressed, HIGH - not pressed
  if (digitalRead(BUTTON_MIDDLE) == HIGH) {
    btnMiddlePressed = false;
  } else {
    if (!btnMiddlePressed) {
      btnMiddlePressed = true;
      if (menuVisible) {
        menuItemClick();
      } else {
        showMenu();
      }
    }
  }
  if (digitalRead((settings.displayRotation == 1) ? BUTTON_RIGHT : BUTTON_LEFT) == HIGH) {
    btnLeftPressed = false;
  } else {
    if (!btnLeftPressed) {
      btnLeftPressed = true;
      // Menu handling
      if (menuVisible) {
        menuMove(false);
      } else {
        displayScreen++;
        if (displayScreen > displayScreenCount)
          displayScreen = 0; // rotate screens
        // Turn off display on screen 0
        digitalWrite(TFT_BL, (displayScreen == 0) ? LOW : HIGH);
        redrawScreen(true);
      }
    }
  }
  if (digitalRead((settings.displayRotation == 1) ? BUTTON_LEFT : BUTTON_RIGHT) == HIGH) {
    btnRightPressed = false;
  } else {
    if (!btnRightPressed) {
      btnRightPressed = true;
      // Menu handling
      if (menuVisible) {
        menuMove(true);
      } else {
        // doAction
      }
    }
  }

  // currentTime & 1ms delay
  struct tm now;
  getLocalTime(&now, 0);
  params.currentTime = mktime(&now);
  delay(1);
}
