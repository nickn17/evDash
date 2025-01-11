#pragma once

//
#include <TinyGPS++.h>
#include "BoardInterface.h"
#include <SD.h>
#include <SPI.h>
#include "SDL_Arduino_INA3221.h"

#ifdef BOARD_M5STACK_CORE2
#include <M5Core2.h>
#define fontRobotoThin24 &Roboto_Thin_24
#define fontOrbitronLight24 &Orbitron_Light_24
#define fontOrbitronLight32 &Orbitron_Light_32
#define fontFont2 &FreeSans9pt7b
#define fontFont7 &FreeSansBold12pt7b

#endif // BOARD_M5STACK_CORE2
#ifdef BOARD_M5STACK_CORES3
#include <M5CoreS3.h>
#define fontRobotoThin24 &fonts::Roboto_Thin_24
#define fontOrbitronLight24 &fonts::Orbitron_Light_24
#define fontOrbitronLight32 &fonts::Orbitron_Light_32
#define fontFont2 &FreeSans9pt7b
#define fontFont7 &FreeSansBold12pt7b
#define fontFont2bmp &fonts::Font2
#define fontFont7bmp &fonts::Font7
#endif // BOARD_M5STACK_CORES3

class Board320_240 : public BoardInterface
{

protected:
// TFT, SD SPI
#if BOARD_M5STACK_CORE2
  M5Display tft;
  TFT_eSprite spr = TFT_eSprite(&tft);
  const GFXfont *lastFont;
  void sprSetFont(const GFXfont *f);
#endif // BOARD_M5STACK_CORE
#if BOARD_M5STACK_CORES3
  M5GFX &tft = CoreS3.Display;
  M5Canvas spr = M5Canvas(&CoreS3.Display);
  const lgfx::GFXfont *lastFont;
  void sprSetFont(const lgfx::GFXfont *font);
#endif // BOARD_M5STACK_CORES3
  void sprDrawString(const char *string, int32_t poX, int32_t poY);
  void tftDrawStringFont7(const char *string, int32_t poX, int32_t poY);
  HardwareSerial *gpsHwUart = NULL;
  SDL_Arduino_INA3221 ina3221;
  TinyGPSPlus gps;
  char tmpStr1[64];
  char tmpStr2[20];
  char tmpStr3[20];
  char tmpStr4[20];
  float lastSpeedKmh = 0;
  int firstReload = 0;
  uint8_t menuVisibleCount = 5;
  uint8_t menuItemHeight = 20;
  time_t lastRedrawTime = 0;
  uint8_t currentBrightness = 255;
  // time in fwd mode for avg speed calc.
  time_t previousForwardDriveModeTotal = 0;
  time_t lastForwardDriveModeStart = 0;
  bool lastForwardDriveMode = false;
  float forwardDriveOdoKmStart = -1;
  float forwardDriveOdoKmLast = -1;
  uint32_t mainLoopStart = 0;
  float displayFps = 0;

public:
  byte pinButtonLeft = 0;
  byte pinButtonRight = 0;
  byte pinButtonMiddle = 0;
  byte pinSpeaker = 0;
  byte pinBrightness = 0;
  byte pinSdcardCs = 0;
  byte pinSdcardMosi = 0;
  byte pinSdcardMiso = 0;
  byte pinSdcardSclk = 0;
  //
  void initBoard() override;
  void afterSetup() override;
  static void xTaskCommLoop(void *pvParameters);
  void commLoop() override;
  void boardLoop() override;
  void mainLoop() override;
  bool skipAdapterScan() override;
  void otaUpdate() override;
  // SD card
  bool sdcardMount() override;
  void sdcardToggleRecording() override;
  // GPS
  void initGPS();
  void syncGPS();
  void syncTimes(time_t newTime);
  void setGpsTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds);
  // Notwork
  bool wifiSetup();
  void netLoop();
  bool netSendData();
  bool netContributeData();
  void wifiFallback();
  void wifiSwitchToMain();
  void wifiSwitchToBackup();
  void uploadSdCardLogToEvDashServer();
  // Basic GUI
  void turnOffScreen() override;
  void setBrightness() override;
  void displayMessage(const char *row1, const char *row2) override;
  bool confirmMessage(const char *row1, const char *row2) override;
  void redrawScreen() override;
  // Custom screens
  void drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, uint16_t bgColor, uint16_t fgColor);
  void drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, int16_t bgColor, int16_t fgColor);
  void showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char *topleft, const char *topright, const char *bottomleft, const char *bottomright, uint16_t color);
  void drawSceneMain();
  void drawSceneSpeed();
  void drawSceneHud();
  void drawSceneBatteryCells();
  void drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, float mulY);
  void drawSceneChargingGraph();
  void drawSceneSoc10Table();
  void drawSceneDebug();
  // Menu
  String menuItemText(int16_t menuItemId, String title);
  void showMenu() override;
  void hideMenu() override;
  void menuMove(bool forward, bool rotate = true);
  void menuItemClick();
  //
  void loadTestData();
  void printHeapMemory();
  //
};
