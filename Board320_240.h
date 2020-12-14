#ifndef BOARD320_240_H
#define BOARD320_240_H

// TFT COMMON
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts
#define SMOOTH_FONT
#define GFXFF 1  // TFT FOnts

//
#include <TFT_eSPI.h>
#include <TinyGPS++.h>
#include "BoardInterface.h"

class Board320_240 : public BoardInterface {

  protected:
    // TFT, SD SPI
    TFT_eSPI tft = TFT_eSPI();
    TFT_eSprite spr = TFT_eSprite(&tft);
    //SPIClass spiSD(HSPI);
    HardwareSerial* gpsHwUart = NULL;
    TinyGPSPlus gps;
    char tmpStr1[20];
    char tmpStr2[20];
    char tmpStr3[20];
    char tmpStr4[20];
  public:
    bool invertDisplay = false;
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
    void mainLoop() override;
    bool skipAdapterScan() override;
    // SD card
    bool sdcardMount() override;
    void sdcardToggleRecording() override;
    // GPS
    void syncGPS();
    // Basic GUI
    void setBrightness(byte lcdBrightnessPerc) override;
    void displayMessage(const char* row1, const char* row2) override;
    void redrawScreen() override;
    // Custom screens
    void drawBigCell(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, uint16_t bgColor, uint16_t fgColor);
    void drawSmallCell(int32_t x, int32_t y, int32_t w, int32_t h, const char* text, const char* desc, int16_t bgColor, int16_t fgColor);
    void showTires(int32_t x, int32_t y, int32_t w, int32_t h, const char* topleft, const char* topright, const char* bottomleft, const char* bottomright, uint16_t color);
    void drawSceneMain();
    void drawSceneSpeed();
    void drawSceneBatteryCells();
    void drawPreDrawnChargingGraphs(int zeroX, int zeroY, int mulX, int mulY);
    void drawSceneChargingGraph();
    void drawSceneSoc10Table();
    void drawSceneDebug();
    // Menu
    String menuItemCaption(int16_t menuItemId, String title);
    void showMenu() override;
    void hideMenu() override;
    void menuMove(bool forward);
    void menuItemClick();
    //
    void loadTestData();
    //
};

#endif // BOARD320_240_H
