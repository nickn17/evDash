#pragma once

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
    void enterSleepMode(int secs) override;
    void wakeupBoard() override;
};

