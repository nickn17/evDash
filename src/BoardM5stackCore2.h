#pragma once

#define BUTTON_LEFT 1
#define BUTTON_MIDDLE 2
#define BUTTON_RIGHT 3

#include "BoardInterface.h"
#include "Board320_240.h"

class BoardM5stackCore2 : public Board320_240 {

  protected:
  public:
    void initBoard() override;
    void wakeupBoard() override;
    void mainLoop() override;
    bool isButtonPressed(int button) override;
    void enterSleepMode(int secs) override;
};
