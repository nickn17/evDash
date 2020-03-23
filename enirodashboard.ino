/*

  KIA eNiro Dashboard

  !! IMPORTANT Replace HM_MAC, serviceUUID, charTxUUID, charRxUUID as described below
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

  --
  Display il9341 is using  TFT_eSPI
  You need to do some user setup in library folder (Adruino/library/tft/espi/userSetup..
  Settings for TFT_eSPI library - userSetup required for T4 v1.3
  #define TFT_DC   32            // v1.3 has DC on 32 port
  #define TFT_BL   4             // Backlight port - required (otherwise you got black screen)
  #define TFT_BACKLIGHT_ON HIGH  // Backlight ON - required
*/

#include "SPI.h"
#include "TFT_eSPI.h"
#include "BLEDevice.h"

// PLEASE CHANGE THIS SETTING for your BLE4
uint32_t PIN = 1234;
#define HM_MAC "dd:0d:30:50:ed:63"  // mac ios-vlink cez nRf connect
static BLEUUID serviceUUID("000018f0-0000-1000-8000-00805f9b34fb"); // nRf connect to ios.vlink / client / dblclick on unknown service - this is service UUID
static BLEUUID  charTxUUID("00002af0-0000-1000-8000-00805f9b34fb"); // UUID from NOTIFY section (one of custom characteristics under unknown service)
static BLEUUID  charRxUUID("00002af1-0000-1000-8000-00805f9b34fb"); // UUID from WRITE section (one of custom characteristics under unknown service)
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
BLEClient*  pClient;

// Temporary variables
char ch;
String line;
char tmpStr1[20];
char tmpStr2[20];
char tmpStr3[20];
char tmpStr4[20];

// Main
byte displayScreen  = 0; // 0 - bash board, 1 - battery cells
bool btnLeftPressed   = true;
bool btnMiddlePressed = true;
bool btnRightPressed  = true;


// Commands loop
#define commandQueueCount 23
#define commandQueueLoopFrom 6
String responseRow;
String responseRowMerged;
byte commandQueueIndex;
bool couldSendNextAtCommand = false;
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
  "atsh7e4",
  "220101",     // power kw, ...
  //"220102",   // cell voltages
  //"220103",   // cell voltages
  //"220104",   // cell voltages
  "220105",     // soh, soc, ..
  //"220106",
  "atsh7e2",
  "2101",       // speed, ...
  "2102",       // aux, ...
  "atsh7df",
  //"2106",
  //"220106",
  "atsh7b3",
  "220100",     // in/out temp
  "atsh7a0",
  "22c00b",     // tire pressure/temp
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
  float batCellMin;
  float batCellMax;
  float batTempC;
  float batHeaterC;
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
};

strucParams params;  // Current
strucParams oldParams;  // Old states used for redraw changed values only

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
  params.batCellMin = -1;
  params.batCellMax = -1;
  params.batTempC = -1;
  params.batHeaterC = -1;
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
bool monitoringRect(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, int16_t color) {

  int32_t posx, posy;

  posx = (x * 80) + 4;
  posy = (y * 60) + 1;

  tft.fillRect(x * 80, y * 60, ((w) * 80) - 1, ((h) * 60) - 1,  color);
  tft.drawFastVLine(((x + w) * 80) - 1, ((y) * 60) - 1, h * 60, TFT_BLACK);
  tft.drawFastHLine(((x) * 80) - 1, ((y + h) * 60) - 1, w * 80, TFT_BLACK);
  tft.setTextDatum(TL_DATUM); // Topleft
  tft.setTextColor(TFT_SILVER, color); // Bk, fg color
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
    tft.setTextColor(TFT_WHITE, color);
    tft.setFreeFont(&Orbitron_Light_32);
    tft.drawString(text, posx, posy, 7);

  } else {

    // All others 1x1 cells
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, color);
    tft.setFreeFont(&Orbitron_Light_24);
    posx = (x * 80) + (w * 80 / 2) - 3;
    posy = (y * 60) + (h * 60 / 2) + 4;
    tft.drawString(text, posx, posy, (w == 2 ? 7 : GFXFF));
  }

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

  // batPowerKwh100 on roads
  if (params.speedKmh > 10) {
    if (force || params.batPowerKwh100 != oldParams.batPowerKwh100) {
      sprintf(tmpStr1, "%01.01f", params.batPowerKwh100);
      monitoringRect(1, 1, 2, 2, tmpStr1, "KWH/100KM", (params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (params.batPowerKw < -16.0 ? TFT_RED : TFT_DARKRED)));
      oldParams.speedKmh = params.batPowerKwh100;
    }
  } else {
    // batPowerAmp on chargers (under 10kmh)
    if (force || params.batPowerAmp != oldParams.batPowerAmp) {
      sprintf(tmpStr1, (abs(params.batPowerAmp) > 9.9 ? "%01.00f" : "%01.01f"), params.batPowerAmp);
      monitoringRect(1, 1, 2, 2, tmpStr1, "BATTERY POWER [A]", (params.batPowerAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED));
      oldParams.batPowerAmp = params.batPowerAmp;
    }
  }

  // socPerc
  if (force || params.socPerc != oldParams.socPerc) {
    sprintf(tmpStr1, "%01.00f%%", params.socPerc);
    sprintf(tmpStr2, (params.sohPerc ==  100.0 ? "SOC/H%01.00f%%" : "SOC/H%01.01f%%"), params.sohPerc);
    monitoringRect(0, 0, 1, 1, tmpStr1, tmpStr2, (params.socPerc < 10 || params.sohPerc < 100 ? TFT_RED : (params.socPerc  > 80 ? TFT_DARKGREEN2 : TFT_DEFAULT_BK)));
    oldParams.socPerc = params.socPerc;
    oldParams.sohPerc = params.sohPerc;
  }

  // batPowerAmp
  if (force || params.batPowerKw != oldParams.batPowerKw) {
    sprintf(tmpStr1, "%01.01f", params.batPowerKw);
    monitoringRect(0, 1, 1, 1, tmpStr1, "POWER KW", (params.batPowerKw >= 0 ? TFT_DARKGREEN2 : (params.batPowerKw <= -30 ? TFT_RED : TFT_DARKRED)));
    oldParams.batPowerKw = params.batPowerKw;
  }

  // batVoltage
  if (force || params.batVoltage != oldParams.batVoltage) {
    sprintf(tmpStr1, "%03.00f", params.batVoltage);
    monitoringRect(0, 2, 1, 1, tmpStr1, "VOLTAGE", TFT_DEFAULT_BK);
    oldParams.batVoltage = params.batVoltage;
  }

  // batCellMin
  if (force || params.batCellMin != oldParams.batCellMin || params.batCellMax != oldParams.batCellMax) {
    sprintf(tmpStr1, "%01.02f", params.batCellMax - params.batCellMin);
    sprintf(tmpStr2, "CELLS %01.02f", params.batCellMin);
    monitoringRect(0, 3, 1, 1, ( params.batCellMax - params.batCellMin == 0.00 ? "OK" : tmpStr1), tmpStr2, TFT_DEFAULT_BK);
    oldParams.batCellMax = params.batCellMax;
    oldParams.batCellMin = params.batCellMin;
  }

  // batTempC
  if (force || params.batTempC != oldParams.batTempC) {
    sprintf(tmpStr1, "%01.00fC", params.batTempC);
    monitoringRect(1, 3, 1, 1, tmpStr1, "BAT.TEMP", TFT_TEMP);
    oldParams.batTempC = params.batTempC;
  }

  // batHeaterC
  if (force || params.batHeaterC != oldParams.batHeaterC) {
    sprintf(tmpStr1, "%01.00fC", params.batHeaterC);
    monitoringRect(2, 3, 1, 1, tmpStr1, "BAT.HEAT", TFT_TEMP);
    oldParams.batHeaterC = params.batHeaterC;
  }

  // Aux perc
  if (force || params.auxPerc != oldParams.auxPerc) {
    sprintf(tmpStr1, "%01.00f%%", params.auxPerc);
    monitoringRect(3, 0, 1, 1, tmpStr1, "AUX BAT.", (params.auxPerc < 60 ? TFT_RED : TFT_DEFAULT_BK));
    oldParams.auxPerc = params.auxPerc;
  }

  // Aux amp
  if (force || params.auxCurrentAmp != oldParams.auxCurrentAmp) {
    sprintf(tmpStr1, (abs(params.auxCurrentAmp) > 9.9 ? "%01.00f" : "%01.01f"), params.auxCurrentAmp);
    monitoringRect(3, 1, 1, 1, tmpStr1, "AUX AMPS",  (params.auxCurrentAmp >= 0 ? TFT_DARKGREEN2 : TFT_DARKRED));
    oldParams.auxCurrentAmp = params.auxCurrentAmp;
  }

  // auxVoltage
  if (force || params.auxVoltage != oldParams.auxVoltage) {
    sprintf(tmpStr1, "%01.01f", params.auxVoltage);
    monitoringRect(3, 2, 1, 1, tmpStr1, "AUX VOLTS", (params.auxVoltage < 12.1 ? TFT_RED : (params.auxVoltage < 12.6 ? TFT_ORANGE : TFT_DEFAULT_BK)));
    oldParams.auxVoltage = params.auxVoltage;
  }

  // indoorTemperature
  if (force || params.indoorTemperature != oldParams.indoorTemperature || params.outdoorTemperature != oldParams.outdoorTemperature) {
    sprintf(tmpStr1, "%01.01f", params.indoorTemperature);
    sprintf(tmpStr2, "IN/OUT%01.01fC", params.outdoorTemperature);
    monitoringRect(3, 3, 1, 1, tmpStr1, tmpStr2, TFT_TEMP);
    oldParams.indoorTemperature = params.indoorTemperature;
    oldParams.outdoorTemperature = params.outdoorTemperature;
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

  // Main screen
  if (displayScreen == 0) {
    drawSceneMain(force);
  }

  // Battery cells
  if (displayScreen == 1) {
    // UNDER CONSTRUCTION
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
    params.batCellMax = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, false) / 50.0;   // === OK Valid
    params.batCellMin = hexToDec(responseRowMerged.substring(56, 58).c_str(), 1, false) / 50.0;   // === OK Valid
    params.batTempC = hexToDec(responseRowMerged.substring(36, 38).c_str(), 1, true);          // === OK Valid
  }
  if (commandRequest.equals("220105")) {
    params.sohPerc = hexToDec(responseRowMerged.substring(56, 60).c_str(), 2, false) / 10.0;      // === OK Valid
    params.socPerc = hexToDec(responseRowMerged.substring(68, 70).c_str(), 1, false) / 2.0;       // === OK Valid
    params.batHeaterC = hexToDec(responseRowMerged.substring(52, 54).c_str(), 1, true);        // === OK Valid
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
  responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCCCBCBCBCBCBCBCBCBAAAA";
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

  redrawScreen(false);
  return true;
}

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
  if ( pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) Serial.println("bleConnected");
  Serial.println(" - bleConnected to server");

  // Remote service
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Found our service");

  // Get characteristics
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charTxUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charTxUUID.toString().c_str());
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Get characteristics
  pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(charRxUUID);
  if (pRemoteCharacteristicWrite == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charRxUUID.toString().c_str());
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
        couldSendNextAtCommand = true;
      }
    }
  }
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

  // Set button pins for input
  pinMode(BUTTON_MIDDLE, INPUT);
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  // Init display
  Serial.println("Init TFT display");
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  redrawScreen(true);

  // Show test data on right button during boot device
  if (digitalRead(BUTTON_RIGHT) == LOW) {
    testData();
  }

  // Start BLE connection
  Serial.println("Start BLE with PIN auth");
  BLEDevice::init("");
  line = "";
}

/**
  Loop
*/
void loop() {

  // Connect BLE device
  if (bleConnect == true) {
    pServerAddress = new BLEAddress(HM_MAC);
    if (connectToServer(*pServerAddress)) {

      bleConnected = true;
      bleConnect = false;
      Serial.println("We are now connected to the BLE device.");

      // Serve first command (ATZ)
      doNextAtCommand();
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
  }

  // Read char from BLE
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

    if (couldSendNextAtCommand) {
      couldSendNextAtCommand = false;
      // Debug
      // Serial.println("DO NEXT AT COMMAND");
      // delay(1000);
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
      if (displayScreen == 2)
        displayScreen = 0; // rotate screens
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



