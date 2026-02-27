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
  static constexpr int16_t menuBackbufferOverscanPx = 20;
  uint8_t spriteColorDepth = 8;
  bool menuBackbufferActive = false;
  bool ensureMenuBackbuffer();
  void releaseMenuBackbuffer();
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
  uint8_t menuItemOffsetPx = 0;
  bool menuDragScrollActive = false;
  int16_t menuTouchHoverIndex = -1;
  uint16_t batteryCellsPage = 0;
  uint8_t debugInfoPage = 0;
  time_t lastRedrawTime = 0;
  uint8_t currentBrightness = 255;
  // time in fwd mode for avg speed calc.
  time_t previousForwardDriveModeTotal = 0;
  time_t lastForwardDriveModeStart = 0;
  bool lastForwardDriveMode = false;
  float forwardDriveOdoKmStart = -1;
  float forwardDriveOdoKmLast = -1;
  uint32_t mainLoopStart = 0;
  uint32_t lastTimeUpdateMs = 0;
  uint32_t suppressTouchInputUntilMs = 0;
  bool keyboardInputActive = false;
  bool messageDialogVisible = false;
  time_t cachedNowEpoch = 0;
  struct tm cachedNow = {};
  float displayFps = 0;
  bool modalDialogActive = false;
  bool screenSwipePreviewActive = false;
  String dismissedCanStatusText = "";
  time_t dismissedNetFailureTime = 0;
  bool lastChargingOn = false;
  uint32_t lastNetSendDurationMs = 0;
  uint32_t wifiTransferredBytes = 0;
  uint32_t lastFirmwareVersionCheckMs = 0;
  bool lastWifiConnected = false;
  uint32_t ntpAttemptStartMs = 0;
  uint32_t ntpLastAttemptMs = 0;
  bool gpsTimeFallbackAllowed = false;
  String lastNotifiedFirmwareVersion = "";
  char pairPendingCode[7] = {0};
  time_t pairPendingExpiresAt = 0;
  uint32_t pairLastStatusPollMs = 0;
  uint8_t pairLastKnownState = 0; // 0-none, 1-pending, 2-paired, 3-expired
  static constexpr uint8_t kContributeSampleSlots = 12;
  static constexpr time_t kContributeSampleIntervalSec = 5;
  static constexpr time_t kContributeSampleWindowSec = 60;
  static constexpr uint32_t kContributeCycleIntervalMs = static_cast<uint32_t>(kContributeSampleWindowSec) * 1000U;
  static constexpr uint32_t kContributeWaitFallbackMs = 4000;
  struct ContributeMotionSample
  {
    time_t time = 0;
    bool hasGpsFix = false;
    float lat = -1.0f;
    float lon = -1.0f;
    float speedKmh = -1.0f;
    float headingDeg = -1.0f;
    float cellMinV = -1.0f;
    float cellMaxV = -1.0f;
    uint8_t cellMinNo = 255;
  };
  struct ContributeChargingSample
  {
    time_t time = 0;
    float soc = -1.0f;
    float batV = -1.0f;
    float batA = -1000.0f;
    float powKw = -1000.0f;
  };
  struct ContributeChargingEvent
  {
    bool valid = false;
    time_t time = 0;
    float soc = -1.0f;
    float batV = -1.0f;
    float batA = -1000.0f;
    float cellMinV = -1.0f;
    float cellMaxV = -1.0f;
    uint8_t cellMinNo = 255;
    float batMinC = -100.0f;
    float batMaxC = -100.0f;
    float cecKWh = -1.0f;
    float cedKWh = -1.0f;
  };
  ContributeMotionSample contributeMotionSamples[kContributeSampleSlots] = {};
  ContributeChargingSample contributeChargingSamples[kContributeSampleSlots] = {};
  uint8_t contributeMotionSampleCount = 0;
  uint8_t contributeMotionSampleNext = 0;
  uint8_t contributeChargingSampleCount = 0;
  uint8_t contributeChargingSampleNext = 0;
  time_t lastContributeSampleTime = 0;
  time_t lastContributeSdRecordTime = 0;
  uint32_t contributeStatusSinceMs = 0;
  uint32_t nextContributeCycleAtMs = 0;
  static constexpr uint32_t kSdV2BackgroundUploadIntervalMs = 5000U;
  static constexpr uint32_t kSdV2BackgroundStartDelayMs = 5U * 60U * 1000U;
  static constexpr uint32_t kSdV2BackgroundRetryBackoffMs = 60U * 60U * 1000U;
  static constexpr uint32_t kSdV2CleanupIntervalMs = 6U * 60U * 60U * 1000U;
  static constexpr time_t kSdV2UploadedRetentionSec = 30 * 24 * 60 * 60;
  uint32_t nextSdV2BackgroundUploadAtMs = 0;
  uint32_t sdV2BackgroundStartAtMs = 0;
  uint32_t nextSdV2CleanupAtMs = 0;
  String sdV2UploadFilePath = "";
  String sdV2UploadFileName = "";
  uint32_t sdV2UploadPart = 0;
  uint32_t sdV2UploadOffset = 0;
  ContributeChargingEvent contributeLastBeforeCharge = {};
  ContributeChargingEvent contributeLastDuringCharge = {};
  ContributeChargingEvent contributeChargingStartEvent = {};
  ContributeChargingEvent contributeChargingEndEvent = {};
  void updateNetAvailability(bool success);
  bool netStatusMessageVisible() const;
  bool isContributeKeyValid(const char *key) const;
  String ensureContributeKey();
  String getHardwareDeviceId() const;
  String getPairDeviceId() const;
  int compareVersionTags(const String &left, const String &right) const;
  void checkFirmwareVersionOnServer();
  bool requestPairingStart(String &outCode, uint32_t &outExpiresInSec);
  uint8_t requestPairingStatus(const String &pairCode, String &outCarName);
  void startEvdashPairing();
  void pollEvdashPairingStatus();
  void addWifiTransferredBytes(size_t bytes);
  void recordContributeSample();
  ContributeChargingEvent captureContributeChargingEventSnapshot(time_t eventTime) const;
  void handleContributeChargingTransitions();
  bool buildContributePayloadV2(String &outJson, bool useReadableTsForSd = false);
  void syncContributeRelativeTimes(time_t offset);
  void runSdV2BackgroundTasks(bool netReady);
  bool ensureSdV2UploadFileSelected(const String &activeLogFilename);
  bool processSdV2UploadChunk();
  void resetSdV2UploadState();
  bool postSdLogChunkToEvDash(const String &fileName, uint32_t part, const uint8_t *data, size_t length, String *responsePayload = nullptr, int *responseCode = nullptr, bool preferManualTimeouts = false);
  bool cleanupUploadedSdV2Logs();

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
  void commLoop() override;
  void boardLoop() override;
  void mainLoop() override;
  bool skipAdapterScan() override;
  void otaUpdate() override;
  // SD card
  bool sdcardMount() override;
  void sdcardToggleRecording() override;
  void sdcardEraseLogs();
  // GPS
  void initGPS();
  void syncGPS();
  void syncTimes(time_t newTime);
  void setGpsTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t seconds);
  // Notwork
  bool wifiSetup();
  void netLoop();
  bool netSendData(bool sendAbrp);
  bool netContributeData();
  void wifiFallback();
  void wifiSwitchToMain();
  void wifiSwitchToBackup();
  void uploadSdCardLogToEvDashServer(bool silent = false);
  void queueAbrpSdLog(const char *payload, size_t length, time_t currentTime, uint64_t operationTimeSec, bool timeSyncWithGps);
  bool wifiScanToMenu();
  bool promptKeyboard(const char *title, String &value, bool mask, uint8_t maxLen = 63);
  bool promptWifiPassword(const char *ssid, String &outPassword, bool isOpenNetwork);
  bool canStatusMessageVisible();
  bool canStatusMessageHitTest(int16_t x, int16_t y);
  void dismissCanStatusMessage();
  void showBootProgress(const char *step, const char *detail, uint16_t bgColor = TFT_BLACK);
  // Basic GUI
  void turnOffScreen() override;
  void setBrightness() override;
  void displayMessage(const char *row1, const char *row2) override;
  void displayMessage(const char *row1, const char *row2, const char *row3);
  bool confirmMessage(const char *row1, const char *row2) override;
  bool drawActiveScreenToSprite();
  void showScreenSwipePreview(int16_t deltaX);
  void redrawScreen() override;
  // Custom screens
  void drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, uint16_t bgColor, uint16_t fgColor);
  void drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char *text, const char *desc, int16_t bgColor, int16_t fgColor);
  void showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char *topleft, const char *topright, const char *bottomleft, const char *bottomright, uint16_t color);
  void drawSceneMain();
  void drawSceneSpeed();
  void drawSceneHud();
  uint16_t batteryCellsCellsPerPage();
  uint16_t batteryCellsPageCount();
  void batteryCellsPageMove(bool forward);
  uint8_t debugInfoPageCount();
  void debugInfoPageMove(bool forward);
  void drawSceneBatteryCells();
  void drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, float mulY);
  void drawSceneChargingGraph();
  void drawSceneSoc10Table();
  void drawSceneDebug();
  void suppressTouchInputFor(uint16_t durationMs = 220);
  bool isTouchInputSuppressed() const;
  bool isKeyboardInputActive() const;
  bool isMessageDialogVisible() const;
  bool dismissMessageDialog();
  // Menu
  uint16_t menuItemsCountCurrent();
  void menuScrollByPixels(int16_t deltaTopPx);
  uint16_t menuItemFromTouchY(int16_t touchY);
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
