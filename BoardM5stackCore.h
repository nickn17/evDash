#ifndef BOARDM5STACKCORE_H
#define BOARDM5STACKCORE_H

// Setup for m5stack core
#define USER_SETUP_LOADED 1
#define SPI_FREQUENCY  27000000
#define SPI_TOUCH_FREQUENCY  2500000
#define USER_SETUP_LOADED 1
#define ILI9341_DRIVER
#define M5STACK
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   14  // Chip select control pin
#define TFT_DC   27  // Data Command control pin
#define TFT_RST  33  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   32  // LED back-light
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  5000000
// BEEP PIN
#define SPEAKER_PIN 25
#define TONE_PIN_CHANNEL 0 

// SDCARD
#define SDCARD_CS    4
#define SDCARD_MOSI  23
#define SDCARD_MISO  19
#define SDCARD_SCLK  18

#define BUTTON_LEFT 37
#define BUTTON_MIDDLE 38
#define BUTTON_RIGHT 39

//
#include "BoardInterface.h"
#include "Board320_240.h"

class BoardM5stackCore : public Board320_240 {

  private:
  public:
    void initBoard() override;
    void mainLoop() override;
};

#endif // BOARDM5STACKCORE_H
