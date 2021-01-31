#ifndef BOARDTTGOT4V13_H
#define BOARDTTGOT4V13_H

// Setup for TTGO T4 v13
#define USER_SETUP_LOADED 1
#define SPI_FREQUENCY  27000000
#define SPI_TOUCH_FREQUENCY  2500000

#define ILI9341_DRIVER
#define TFT_MISO 12
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   27
#define TFT_DC   32
#define TFT_RST   5
//#define TFT_BACKLIGHT_ON HIGH
#define TFT_BL 4

#define USE_HSPI_PORT
//#define SPI_FREQUENCY  40000000   // Maximum for ILI9341
#define SPI_READ_FREQUENCY  6000000 // 6 MHz is the maximum SPI read speed for the ST7789V

#define SDCARD_CS    13
#define SDCARD_MOSI  15
#define SDCARD_MISO  2
#define SDCARD_SCLK  14

#define BUTTON_LEFT 38
#define BUTTON_MIDDLE 37
#define BUTTON_RIGHT 39

//
#include "BoardInterface.h"
#include "Board320_240.h"

//
class BoardTtgoT4v13 : public Board320_240 {
  
  protected:   
  public:
    void initBoard() override;
    bool isButtonPressed(int button) override;
};

#endif // BOARDTTGOT4V13_H
