/*

  KIA eNiro Dashboard 1.01, 2020-04-12
  working only with OBD BLE 4.0 adapters ex. Vgate ICar Pro (BLE4.0 version)

  IMPORTANT Replace HM_MAC, serviceUUID, charTxUUID, charRxUUID as described below

  !! How to obtain MAC + 3x UUID? (I want to add pairing via buttons later)

  Run Android BLE scanner
  - choose IOS-VLINK device
  - get mac address a replace HM_MAC constant, then open CUSTOM service (first of 2)
  - there is serviceUUID (replace bellow in code)
  - open it.. there are 2x custom characteristics (first is for NOTIFY (read), and second for WRITE,WRITE_REQUEST).
  set charTxUUID with UUID from NOTIFY section
  set charRxUUID with UUID from WRITE section

  Example.
  #define HM_MAC "dd:0d:30:50:ed:63"
  static BLEUUID serviceUUID("000018f0-0000-1000-8000-00805f9b34fb");
  static BLEUUID  charTxUUID("00002af0-0000-1000-8000-00805f9b34fb");
  static BLEUUID  charRxUUID("00002af1-0000-1000-8000-00805f9b34fb");
*/

#include "SPI.h"
#include "TFT_eSPI.h"
#include "BLEDevice.h"

// PLEASE CHANGE THIS SETTING for your BLE4
uint32_t PIN = 1234;
// Temporary moved to initSettings(). Preparing to load/save settings from flash memory
//#define HM_MAC "dd:0d:30:50:ed:63"  // mac ios-vlink cez nRf connect
//static BLEUUID serviceUUID("000018f0-0000-1000-8000-00805f9b34fb"); // nRf connect to ios.vlink / client / dblclick on unknown service - this is service UUID
//static BLEUUID  charTxUUID("00002af0-0000-1000-8000-00805f9b34fb"); // UUID from NOTIFY section (one of custom characteristics under unknown service)
//static BLEUUID  charRxUUID("00002af1-0000-1000-8000-00805f9b34fb"); // UUID from WRITE section (one of custom characteristics under unknown service)
///////////////////////////////////////////////

// LILYGO TTGO T4 v1.3 BUTTONS
#define BUTTON_MIDDLE 37
#define BUTTON_LEFT 38
#define BUTTON_RIGHT 39

/* TFT COLORS */
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_DEFAULT_BK     0x0000    // 0x38E0
#define TFT_TEMP        0x0000    // NAVY
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_DARKRED     0x3800      /* 128,   0,   0 */
#define TFT_DARKGREEN2  0x01E0      /* 128,   0,   0 */

// Misc
#define GFXFF 1  // TFT FOnts
#define PSI2BAR_DIVIDER  14.503773800722  // tires psi / 14,503773800722 -> bar

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

// Main
#define displayScreenCount 4
byte displayScreen  = 1; // 0 - blank screen, 1 - dash board (default), 2 - big speed + kwh/100, 3 - battery cells, 4 - charging graph
bool btnLeftPressed   = true;
bool btnMiddlePressed = true;
bool btnRightPressed  = true;

// Commands loop
#define commandQueueCount 23
#define commandQueueLoopFrom 7
String responseRow;
String responseRowMerged;
byte commandQueueIndex;
bool canSendNextAtCommand = false;
String commandRequest = "";
String commandQueue[commandQueueCount] = {
  "AT Z",      // Reset all
  "AT I",      // Print the version ID
  "AT E0",     // Echo off
  "AT L0",     // Linefeeds off
  //"AT SP 6",   // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
  //"AT AL",     // Allow Long (>7 byte) messages
  //"AT AR",     // Automatically receive
  //"AT H1",     // Headers on (debug only)
  "AT S0",     // Printing of spaces on
  //"AT D1",     // Display of the DLC on
  //"AT CAF0",   // Automatic formatting off
  "AT DP",
  "atst16",

  // Loop from (KIA ENIRO)
  "atsh7e4",  // BMS
  "220101",   // power kw, ...
  "220102",   // cell voltages, screen 3 only
  "220103",   // cell voltages, screen 3 only
  "220104",   // cell voltages, screen 3 only
  "220105",    // soh, soc, ..
  //"220106",
  
  "atsh7e2",  // VMCU
  "2101",     // speed, ...
  "2102",     // aux, ...
  
  //"atsh7df",
  //"2106",
  //"220106",
  
  "atsh7b3",   
  "220100",   // in/out temp
  
  "atsh7a0",  // TMPS?
  "22c00b",   // tire pressure/temp
};

// Menu id/parent/title
typedef struct {
  uint8_t lang;
  char* sound;
  char* value;
} menuItem;

String menu[] = {
  "1/0/Vehicle type",
  "100/1/Kia eNiro",
  "101/1/Hyundai Kona EV",
  "2/0/Pair OBD2 BLE adapter",
  "3/0/Other",
  "30/3/Screen rotation",
  "300/30/Normal",
  "301/30/Flip vertical",
  "4/0/Units",
  "40/4/Distance",
  "400/40/Kilometers",
  "401/40/Miles",
  "41/4/Temperature",
  "410/41/Celsius",
  "411/41/Fahrenheit",
  "42/4/Pressure",
  "420/42/Bar",
  "421/42/Psi",
};

// Structure with realtime values
struct strucParams {
  float speedKmh;
  float socPerc;
  float sohPerc;
  float cumulativeEnergyChargedKWh;
  float cumulativeEnergyChargedKWhStart;
  float cumulativeEnergyDischargedKWh;
  float cumulativeEnergyDischargedKWhStart;
  float batPowerAmp;
  float batPowerKw;
  float batPowerKwh100;
  float batVoltage;
  float batCellMinV;
  float batCellMaxV;
  float batTempC;
  float batHeaterC;
  float batInletC;
  float batMinC;
  float batMaxC;
  float batModule01TempC;
  float batModule02TempC;
  float batModule03TempC;
  float batModule04TempC;
  float auxPerc;
  float auxCurrentAmp;
  float auxVoltage;
  float indoorTemperature;
  float outdoorTemperature;
  float tireFrontLeftTempC;
  float tireFrontLeftPressureBar;
  float tireFrontRightTempC;
  float tireFrontRightPressureBar;
  float tireRearLeftTempC;
  float tireRearLeftPressureBar;
  float tireRearRightTempC;
  float tireRearRightPressureBar;
  float cellVoltage[98]; // 1..98 has index 0..97
  float chargingGraphKw[101]; // 0..100% .. how many HW in each step
  float chargingGraphMinTempC[101]; // 0..100% .. Min temp in.C
  float chargingGraphMaxTempC[101]; // 0..100% .. Max temp in.C
};

// Setting stored to flash
struct strucSettings {
  byte initFlag; // 183 value
  byte settingsVersion; // 1
  char obdMacAddress[20];
  char serviceUUID[40];
  char charTxUUID[40];
  char charRxUUID[40];
};

strucParams params;     // Realtime sensor values
strucParams oldParams;  // Old states used for change detection (draw optimization)
strucSettings settings; // Settings stored into flash

/**
   Init settings
*/
bool initSettings() {

  String tmpStr;

  settings.initFlag = 183;
  settings.settingsVersion = 1;
  tmpStr = "dd:0d:30:50:ed:63";
  tmpStr.toCharArray(settings.obdMacAddress, 18);
  tmpStr = "000018f0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(settings.serviceUUID, 37);
  tmpStr  = "00002af0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(settings.charTxUUID, 37);
  tmpStr  = "00002af1-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(settings.charRxUUID, 37);

  return true;
}

/**
   Init structure with data
*/
bool initStructure() {

  params.speedKmh = -1;
  params.socPerc = -1;
  params.sohPerc = -1;
  params.cumulativeEnergyChargedKWh = -1;
  params.cumulativeEnergyChargedKWhStart = -1;
  params.cumulativeEnergyDischargedKWh = -1;
  params.cumulativeEnergyDischargedKWhStart = -1;
  params.batPowerAmp = -1;
  params.batPowerKw = -1;
  params.batPowerKwh100 = -1;
  params.batVoltage = -1;
  params.batCellMinV = -1;
  params.batCellMaxV = -1;
  params.batTempC = -1;
  params.batHeaterC = -1;
  params.batInletC = -1;
  params.batMinC = -1;
  params.batMaxC = -1;
  params.batModule01TempC = -1;
  params.batModule02TempC = -1;
  params.batModule03TempC = -1;
  params.batModule04TempC = -1;
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
  for (int i = 0; i < 98; i++) {
    params.cellVoltage[i] = 0;
  }
  for (int i = 0; i <= 100; i++) {
    params.chargingGraphKw[i] = 0;
    params.chargingGraphMinTempC[i] = -100;
    params.chargingGraphMaxTempC[i] = -100;
  }

  oldParams = params;

  return true;
}

/**
  Hex to dec (1-2 byte values, signed/unsigned)
  For 4 byte change int to long and add part for signed numbers
*/
float hexToDec(String hexString, byte bytes = 2, bool signedNum = true) {

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
   Main screen (Screen 0)
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
    sprintf(tmpStr1, "%01.01fbar %02.00fC", params.tireFrontLeftPressureBar, params.tireFrontLeftTempC);
    sprintf(tmpStr2, "%02.00fC %01.01fbar", params.tireFrontRightTempC, params.tireFrontRightPressureBar);
    sprintf(tmpStr3, "%01.01fbar %02.00fC", params.tireRearLeftPressureBar, params.tireRearLeftTempC);
    sprintf(tmpStr4, "%02.00fC %01.01fbar", params.tireRearRightTempC, params.tireRearRightPressureBar);
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
      sprintf(tmpStr1, "%01.01f", params.batPowerKwh100);
      monitoringRect(1, 1, 2, 2, tmpStr1, "KWH/100KM", (params.batPowerKwh100 >= 0 ? TFT_DARKGREEN2 : (params.batPowerKwh100 < -30.0 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
      oldParams.speedKmh = params.batPowerKwh100;
    }
  } else {
    // batPowerAmp on chargers (under 10kmh)
    if (force || params.batPowerAmp != oldParams.batPowerAmp) {
      sprintf(tmpStr1, (abs(params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), params.batPowerAmp);
      monitoringRect(1, 1, 2, 2, tmpStr1, "BATTERY POWER [A]", (params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED), TFT_WHITE);
      oldParams.batPowerAmp = params.batPowerAmp;
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
  if (force || params.batPowerKw != oldParams.batPowerKw) {
    sprintf(tmpStr1, "%01.01f", params.batPowerKw);
    monitoringRect(0, 1, 1, 1, tmpStr1, "POWER KW", (params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)), TFT_WHITE);
    oldParams.batPowerKw = params.batPowerKw;
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
    sprintf(tmpStr1, "%01.00f", params.batTempC);
    monitoringRect(1, 3, 1, 1, tmpStr1, "BAT.TEMP.C", TFT_TEMP, (params.batTempC >= 15) ? ((params.batTempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
    oldParams.batTempC = params.batTempC;
  }

  // batHeaterC
  if (force || params.batHeaterC != oldParams.batHeaterC) {
    sprintf(tmpStr1, "%01.00f", params.batHeaterC);
    monitoringRect(2, 3, 1, 1, tmpStr1, "BAT.HEAT C", TFT_TEMP, TFT_WHITE);
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
    sprintf(tmpStr1, "%01.01f", params.indoorTemperature);
    sprintf(tmpStr2, "IN/OUT%01.01fC", params.outdoorTemperature);
    monitoringRect(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP, TFT_WHITE);
    oldParams.indoorTemperature = params.indoorTemperature;
    oldParams.outdoorTemperature = params.outdoorTemperature;
  }

  return true;
}

/**
   Speed + kwh/100km (Screen 1)
*/
bool drawSceneSpeed(bool force) {

  int32_t posx, posy;

  posx = 320 / 2;
  posy = 32;
  tft.setTextDatum(TC_DATUM); // Top center
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2); // Size for small 5x7 font
  sprintf(tmpStr3, "     0     ");
  if (params.speedKmh > 10)
    sprintf(tmpStr3, "    %01.00f    ", params.speedKmh);
  tft.drawString(tmpStr3, posx, posy, 7);

  posy = 140;
  tft.setTextDatum(TC_DATUM); // Top center
  tft.setTextSize(1);
  if (params.speedKmh > 25 && params.batPowerKw > 0) {
    sprintf(tmpStr3, "     %01.01f     ", params.batPowerKwh100);
  } else {
    sprintf(tmpStr3, "     %01.01f     ", params.batPowerKw);
  }
  tft.drawString(tmpStr3, posx, posy, 7);
  
  // Bottom 2 numbers with charged/discharged kWh from start
  posx = 10;
  posy = 240 - 10;
  sprintf(tmpStr3, "-%01.01f    ", params.cumulativeEnergyDischargedKWh - params.cumulativeEnergyDischargedKWhStart);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(BL_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);

  posx = 320 - 10;
  sprintf(tmpStr3, "    +%01.01f", params.cumulativeEnergyChargedKWh - params.cumulativeEnergyChargedKWhStart);
  tft.setTextDatum(BR_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);
  
  posx = 320 / 2;
  sprintf(tmpStr3, "   %01.01fkw   ", params.batPowerKw);
  tft.setTextDatum(BC_DATUM);
  tft.drawString(tmpStr3, posx, posy, GFXFF);
  
  // Battery "cold gate" detection - red < 15C (43KW limit), <25 (blue - 55kW limit), green all ok
  sprintf(tmpStr3, "%01.00f", params.batTempC);
  tft.fillCircle(290, 30, 25, (params.batTempC >= 15) ? ((params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  tft.setTextColor(TFT_WHITE, (params.batTempC >= 15) ? ((params.batTempC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(tmpStr3, 290, 30, GFXFF);

  return true;
}

/**
   Battery cells (Screen 2)
*/
bool drawSceneBatteryCells(bool force) {

  int32_t posx, posy;

  sprintf(tmpStr1, "%01.00f C", params.batHeaterC);
  drawSmallRect(0, 0, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.00f C", params.batInletC);
  drawSmallRect(1, 0, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  /*Not needed yet
  sprintf(tmpStr1, "%01.00fC", params.batMinC);
  drawSmallRect(2, 0, 1, 1, tmpStr1, "BAT.MIN", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.00fC", params.batMaxC);
  drawSmallRect(3, 0, 1, 1, tmpStr1, "BAT.MAX", TFT_TEMP, TFT_CYAN);*/

  sprintf(tmpStr1, "%01.00f C", params.batModule01TempC);
  drawSmallRect(0, 1, 1, 1, tmpStr1, "MO1", TFT_TEMP, (params.batModule01TempC >= 15) ? ((params.batModule01TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, "%01.00f C", params.batModule02TempC);
  drawSmallRect(1, 1, 1, 1, tmpStr1, "MO2", TFT_TEMP, (params.batModule02TempC >= 15) ? ((params.batModule02TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, "%01.00f C", params.batModule03TempC);
  drawSmallRect(2, 1, 1, 1, tmpStr1, "MO3", TFT_TEMP, (params.batModule03TempC >= 15) ? ((params.batModule03TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);
  sprintf(tmpStr1, "%01.00f C", params.batModule04TempC);
  drawSmallRect(3, 1, 1, 1, tmpStr1, "MO4", TFT_TEMP, (params.batModule04TempC >= 15) ? ((params.batModule04TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED);

  tft.setTextDatum(TL_DATUM); // Topleft
  tft.setTextSize(1); // Size for small 5x7 font

  // Find min and max val
  float minVal = -1, maxVal = -1;
  for (int i = 0; i < 98; i++) {
    if (params.cellVoltage[i] < minVal || minVal == -1)
      minVal = params.cellVoltage[i];
    if (params.cellVoltage[i] > maxVal || maxVal == -1)
      maxVal = params.cellVoltage[i];
  }

  // Draw cell matrix
  for (int i = 0; i < 98; i++) {
    posx = ((i % 8) * 40) + 4;
    posy = ((floor(i / 8)) * 13) + 68;
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
   Charging graph (Screen 3)
*/
bool drawSceneChargingGraph(bool force) {

  int zeroX = 0;
  int zeroY = 238;
  int mulX = 3; // 100% = 300px;
  int mulY = 2; // 100kW = 200px
  int maxKw = 80;
  int16_t color;

  sprintf(tmpStr1, "%01.00f", params.socPerc);
  drawSmallRect(0, 0, 1, 1, tmpStr1, "SOC", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", params.batPowerKw);
  drawSmallRect(1, 0, 1, 1, tmpStr1, "POWER kW", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.01f", params.batPowerAmp);
  drawSmallRect(2, 0, 1, 1, tmpStr1, "POWER A", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%03.00f", params.batVoltage);
  drawSmallRect(3, 0, 1, 1, tmpStr1, "VOLTAGE", TFT_TEMP, TFT_CYAN);

  sprintf(tmpStr1, "%01.00f C", params.batHeaterC);
  drawSmallRect(0, 1, 1, 1, tmpStr1, "HEATER", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.00f C", params.batInletC);
  drawSmallRect(1, 1, 1, 1, tmpStr1, "BAT.INLET", TFT_TEMP, TFT_CYAN);
  sprintf(tmpStr1, "%01.00f C", params.batMinC);
  drawSmallRect(2, 1, 1, 1, tmpStr1, "BAT.MIN", (params.batMinC >= 15) ? ((params.batMinC >= 25) ? TFT_DARKGREEN2 : TFT_BLUE) : TFT_RED, TFT_CYAN);
  sprintf(tmpStr1, "%01.00f C", params.batMaxC);
  drawSmallRect(3, 1, 1, 1, tmpStr1, "BAT.MAX", TFT_TEMP, TFT_CYAN);

  tft.setTextSize(1); // Size for small 5x7 font
  tft.setTextDatum(TR_DATUM);
  sprintf(tmpStr1, "%01.00fC", params.batModule01TempC);
  tft.setTextColor((params.batModule01TempC >= 15) ? ((params.batModule01TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + 00, 2);
  sprintf(tmpStr1, "%01.00fC", params.batModule02TempC);
  tft.setTextColor((params.batModule02TempC >= 15) ? ((params.batModule02TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + 20, 2);
  sprintf(tmpStr1, "%01.00fC", params.batModule03TempC);
  tft.setTextColor((params.batModule03TempC >= 15) ? ((params.batModule03TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + 40, 2);
  sprintf(tmpStr1, "%01.00fC", params.batModule04TempC);
  tft.setTextColor((params.batModule04TempC >= 15) ? ((params.batModule04TempC >= 25) ? TFT_GREEN : TFT_BLUE) : TFT_RED, TFT_TEMP);
  tft.drawString(tmpStr1, zeroX + (10 * 10 * mulX),  zeroY - (maxKw * mulY) + 60, 2);
  tft.setTextColor(TFT_SILVER, TFT_TEMP);
  
  
  for (int i = 0; i <= 10; i++) {
    color = TFT_DARKRED;
    if (i == 0 || i == 5 || i == 10)
      color = TFT_NAVY;
    tft.drawFastVLine(zeroX + (i * 10 * mulX), zeroY - (maxKw * mulY), maxKw * mulY, color);
    if (i != 0 && i != 10) {
      sprintf(tmpStr1, "%d%%", i*10);
      tft.setTextDatum(BC_DATUM);
      tft.drawString(tmpStr1, zeroX + (i * 10 * mulX),  zeroY - (maxKw * mulY), 2);
    }
    if (i <= (maxKw / 10)) {
      tft.drawFastHLine(zeroX, zeroY - (i * 10 * mulY), 100 * mulX, color);
      if (i > 0) {
        sprintf(tmpStr1, "%d", i*10);
        tft.setTextDatum(ML_DATUM);
        tft.drawString(tmpStr1, zeroX + (100 * mulX)+ 3, zeroY - (i * 10 * mulY), 2);
      }
    }
  }  

  for (int i = 0; i <= 100; i++) {
    if (params.chargingGraphKw[i] > 0)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphKw[i]*mulY), mulX, TFT_YELLOW);
    if (params.chargingGraphMinTempC[i] > -10)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphMinTempC[i]*mulY), mulX, TFT_RED);
    if (params.chargingGraphMaxTempC[i] > -10)
      tft.drawFastHLine(zeroX + (i * mulX) - (mulX / 2), zeroY - (params.chargingGraphMaxTempC[i]*mulY), mulX, TFT_BLUE);
  }

  return true;
}

/**
   Redraw screen
*/
bool redrawScreen(bool force) {

  // Clear screen if needed
  if (force) {
    tft.fillScreen(TFT_BLACK);
  }

  // 1. Main screen
  if (displayScreen == 1) {
    drawSceneMain(force);
  }
  // 2. Big speed + kwh/100km
  if (displayScreen == 2) {
    drawSceneSpeed(force);
  }
  // 3. Battery cells
  if (displayScreen == 3) {
    drawSceneBatteryCells(force);
  }
  // Charging graph
  if (displayScreen == 4) {
    drawSceneChargingGraph(force);
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

  if (commandRequest.equals("2101")) {
    params.speedKmh = hexToDec(responseRowMerged.substring(32, 36).c_str(), 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
  }
  if (commandRequest.equals("2102")) {
    params.auxPerc = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, false);           // === OK Valid
    params.auxCurrentAmp = - hexToDec(responseRowMerged.substring(46, 50).c_str(), 2, true) / 1000.0;
  }
  if (commandRequest.equals("220100")) {
    params.indoorTemperature = (hexToDec(responseRowMerged.substring(16, 18).c_str(), 1, false) / 2) - 40; // === OK Valid
    params.outdoorTemperature = (hexToDec(responseRowMerged.substring(18, 20).c_str(), 1, false) / 2) - 40; // === OK Valid
  }
  if (commandRequest.equals("220101")) {
    params.cumulativeEnergyChargedKWh = float(strtol(responseRowMerged.substring(82, 90).c_str(), 0, 16)) / 10.0;
    if (params.cumulativeEnergyChargedKWhStart == -1)
      params.cumulativeEnergyChargedKWhStart = params.cumulativeEnergyChargedKWh;
    params.cumulativeEnergyDischargedKWh = float(strtol(responseRowMerged.substring(90, 98).c_str(), 0, 16)) / 10.0;
    if (params.cumulativeEnergyDischargedKWhStart == -1)
      params.cumulativeEnergyDischargedKWhStart = params.cumulativeEnergyDischargedKWh;
    params.auxVoltage = hexToDec(responseRowMerged.substring(64, 66).c_str(), 2, true) / 10.0;
    params.batPowerAmp = - hexToDec(responseRowMerged.substring(26, 30).c_str(), 2, true) / 10.0;
    params.batVoltage = hexToDec(responseRowMerged.substring(30, 34).c_str(), 2, false) / 10.0;   // === OK Valid
    params.batPowerKw = (params.batPowerAmp * params.batVoltage) / 1000.0;
    params.batPowerKwh100 = params.batPowerKw / params.speedKmh * 100;
    params.batCellMaxV = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, false) / 50.0;   // === OK Valid
    params.batCellMinV = hexToDec(responseRowMerged.substring(56, 58).c_str(), 1, false) / 50.0;   // === OK Valid
    params.batModule01TempC = hexToDec(responseRowMerged.substring(38, 40).c_str(), 1, true);
    params.batModule02TempC = hexToDec(responseRowMerged.substring(40, 42).c_str(), 1, true);
    params.batModule03TempC = hexToDec(responseRowMerged.substring(42, 44).c_str(), 1, true);
    params.batModule04TempC = hexToDec(responseRowMerged.substring(44, 46).c_str(), 1, true);
    //params.batTempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    //params.batMaxC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);
    //params.batMinC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    
    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    params.batMinC = params.batMaxC = params.batModule01TempC;
    params.batMinC = (params.batModule02TempC < params.batMinC) ? params.batModule02TempC : params.batMinC ;
    params.batMinC = (params.batModule03TempC < params.batMinC) ? params.batModule03TempC : params.batMinC ;
    params.batMinC = (params.batModule04TempC < params.batMinC) ? params.batModule04TempC : params.batMinC ;
    params.batMaxC = (params.batModule02TempC > params.batMaxC) ? params.batModule02TempC : params.batMaxC ;
    params.batMaxC = (params.batModule03TempC > params.batMaxC) ? params.batModule03TempC : params.batMaxC ;
    params.batMaxC = (params.batModule04TempC > params.batMaxC) ? params.batModule04TempC : params.batMaxC ;
    params.batTempC = params.batMinC;
    
    params.batInletC = hexToDec(responseRowMerged.substring(50, 52).c_str(), 1, true);
    if (params.speedKmh < 15 && params.batPowerKw >= 1 && params.socPerc > 0 && params.socPerc <= 100) {
      params.chargingGraphKw[int(params.socPerc)] = params.batPowerKw;
      params.chargingGraphMinTempC[int(params.socPerc)] = params.batMinC;
      params.chargingGraphMaxTempC[int(params.socPerc)] = params.batMaxC;
    }
  }
  if (commandRequest.equals("220102")) {
    for (int i = 0; i < 32; i++) {
      params.cellVoltage[i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  if (commandRequest.equals("220103")) {
    for (int i = 0; i < 32; i++) {
      params.cellVoltage[32 + i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  if (commandRequest.equals("220104")) {
    for (int i = 0; i < 32; i++) {
      params.cellVoltage[64 + i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  if (commandRequest.equals("220105")) {
    params.sohPerc = hexToDec(responseRowMerged.substring(56, 60).c_str(), 2, false) / 10.0;
    params.socPerc = hexToDec(responseRowMerged.substring(68, 70).c_str(), 1, false) / 2.0;
    params.batHeaterC = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, true);
    //
    for (int i = 30; i < 32; i++) { // ai/aj position
      params.cellVoltage[96 - 30 + i] = hexToDec(responseRowMerged.substring(14 + (i * 2), 14 + (i * 2) + 2).c_str(), 1, false) / 50;
    }
  }
  if (commandRequest.equals("22c00b")) {
    params.tireFrontLeftPressureBar = hexToDec(responseRowMerged.substring(14, 16).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
    params.tireFrontRightPressureBar = hexToDec(responseRowMerged.substring(22, 24).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
    params.tireRearLeftPressureBar = hexToDec(responseRowMerged.substring(30, 32).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
    params.tireRearRightPressureBar = hexToDec(responseRowMerged.substring(38, 40).c_str(), 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
    params.tireRearLeftTempC = hexToDec(responseRowMerged.substring(16, 18).c_str(), 2, false)  - 50;      // === OK Valid
    params.tireRearRightTempC = hexToDec(responseRowMerged.substring(24, 26).c_str(), 2, false) - 50;      // === OK Valid
    params.tireFrontLeftTempC = hexToDec(responseRowMerged.substring(32, 34).c_str(), 2, false) - 50;     // === OK Valid
    params.tireFrontRightTempC = hexToDec(responseRowMerged.substring(40, 42).c_str(), 2, false) - 50;     // === OK Valid
  }

  return true;
}

/**
   Parse test data
*/
bool testData() {

  redrawScreen(true);

  // 2101
  commandRequest = "2101";
  responseRowMerged = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
  parseRowMerged();

  // 2102
  commandRequest = "2102";
  responseRowMerged = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
  responseRowMerged = "6102F8FFFC000101000000931CC77F4C39040BE09BA7385D8158832175000001010100000007000000";
  parseRowMerged();

  // 2106
  commandRequest = "2106";
  responseRowMerged = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
  parseRowMerged();

  // 220100
  commandRequest = "220100";
  responseRowMerged = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
  responseRowMerged = "6201007E5027C8FF867C58121010FFFF10FF8EFFFFFFFFFFFF10FFFF0DF0617900FFFF01FFFF000000";
  parseRowMerged();

  // 220101
  commandRequest = "220101";
  responseRowMerged = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
  responseRowMerged = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
  parseRowMerged();

  // 220102
  commandRequest = "220102";
  responseRowMerged = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();

  // 220103
  commandRequest = "220103";
  responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();

  // 220104
  commandRequest = "220104";
  responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();

  // 220105
  commandRequest = "220105";
  responseRowMerged = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
  responseRowMerged = "620105003FFF90000000000000000014918E012927465000015013BB03E800000000BB0000CBCB01001300000000AAAA";
  parseRowMerged();

  // 220106
  commandRequest = "220106";
  responseRowMerged = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
  parseRowMerged();

  // 22c002
  commandRequest = "22c002";
  responseRowMerged = "62C002FFFF0000D2E84E93D2E84EBBD2DBDACBD2E149F3AAAAAAAA";
  parseRowMerged();

  // 22c00b
  commandRequest = "22c00b";
  responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  parseRowMerged();

    params.batModule01TempC = 28;
    params.batModule02TempC = 29;
    params.batModule03TempC = 28;
    params.batModule04TempC = 30;
    //params.batTempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    //params.batMaxC = hexToDec(responseRowMerged.substring(34, 36).c_str(), 1, true);
    //params.batMinC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);
    
    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    params.batMinC = params.batMaxC = params.batModule01TempC;
    params.batMinC = (params.batModule02TempC < params.batMinC) ? params.batModule02TempC : params.batMinC ;
    params.batMinC = (params.batModule03TempC < params.batMinC) ? params.batModule03TempC : params.batMinC ;
    params.batMinC = (params.batModule04TempC < params.batMinC) ? params.batModule04TempC : params.batMinC ;
    params.batMaxC = (params.batModule02TempC > params.batMaxC) ? params.batModule02TempC : params.batMaxC ;
    params.batMaxC = (params.batModule03TempC > params.batMaxC) ? params.batModule03TempC : params.batMaxC ;
    params.batMaxC = (params.batModule04TempC > params.batMaxC) ? params.batModule04TempC : params.batMaxC ;
    params.batTempC = params.batMinC;

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

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(settings.serviceUUID))) {
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

  Serial.print("bleConnect ");
  Serial.println(pAddress.toString().c_str());

  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); //
  pSecurity->setCapability(ESP_IO_CAP_KBDISP);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  if ( pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) Serial.println("bleConnected");
  Serial.println(" - bleConnected to server");

  // Remote service
  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(settings.serviceUUID));
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(settings.serviceUUID);
    return false;
  }
  Serial.println(" - Found our service");

  // Get characteristics
  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(settings.charTxUUID));
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(settings.charTxUUID);//.toString().c_str());
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Get characteristics
  pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(BLEUUID(settings.charRxUUID));
  if (pRemoteCharacteristicWrite == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(settings.charRxUUID);//.toString().c_str());
    return false;
  }
  Serial.println(" - Found our characteristic write");

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
  int32_t posx, posy;

  // Print message
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(&Roboto_Thin_24);
  tft.setTextDatum(BL_DATUM);
  tft.drawString(" Searching BLE device... ", 0, 240 / 2, GFXFF);  

  // Start scanning
  Serial.println("Scanning BLE devices...");
  BLEScanResults foundDevices = pBLEScan->start(5, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  // Redraw screen
  redrawScreen(true);

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
  initSettings();

  // Set button pins for input
  pinMode(BUTTON_MIDDLE, INPUT);
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  // Init display
  Serial.println("Init TFT display");
  tft.begin();
  tft.setRotation(3 );
  tft.fillScreen(TFT_BLACK);
  redrawScreen(true);

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
  // Specify that we want active scanning and start the scan to run for 5 seconds.
  Serial.println("Setup BLE scan");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  startBleScan();

  // End
  Serial.println("Device setup completed");
}

/**
  Loop
*/
void loop() {

  //Serial.println("Loop");

  // Connect BLE device
  if (bleConnect == true && foundMyBleDevice) {
    pServerAddress = new BLEAddress(settings.obdMacAddress);
    if (connectToServer(*pServerAddress)) {

      bleConnected = true;
      bleConnect = false;
      Serial.println("We are now connected to the BLE device.");
      redrawScreen(true);

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
     // doAction
    }
  }
  if (digitalRead(BUTTON_LEFT) == HIGH) {
    btnLeftPressed = false;
  } else {
    if (!btnLeftPressed) {
      btnLeftPressed = true;
      displayScreen++;
      if (displayScreen > displayScreenCount)
        displayScreen = 0; // rotate screens
      // Turn off display on screen 0
      digitalWrite(TFT_BL, (displayScreen == 0) ? LOW : HIGH); 
      redrawScreen(true);
    }
  }
  if (digitalRead(BUTTON_RIGHT) == HIGH) {
    btnRightPressed = false;
  } else {
    if (!btnRightPressed) {
      btnRightPressed = true;
      // doAction
    }
  }

  // 1ms delay
  delay(1);
}
