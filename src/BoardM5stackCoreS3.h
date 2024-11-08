#pragma once

#define BUTTON_LEFT 1
#define BUTTON_MIDDLE 2
#define BUTTON_RIGHT 3

#define BACKGROUND 0
#define BUTTON 1

#include "BoardInterface.h"
#include "Board320_240.h"

class BoardM5stackCoreS3 : public Board320_240
{
protected:
public:
  void initBoard() override;
  void afterSetup() override;
  void boardLoop() override;
  void mainLoop() override;
  bool isButtonPressed(int button) override;
  void enterSleepMode(int secs) override;
  bool skipAdapterScan() override;
  //  static void eventDisplay(Event &e);
  void setTime(String timestamp) override;
  void ntpSync() override;
};
