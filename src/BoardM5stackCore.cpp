#ifdef BOARD_M5STACK_CORE

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore.h"

/**
  Init board
*/
void BoardM5stackCore::initBoard() {

  invertDisplay = true;
  
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;

  M5.begin();

  //
  Board320_240::initBoard();
}

bool BoardM5stackCore::isButtonPressed(int button) {
  switch (button)
  {
  case BUTTON_LEFT:
    return M5.BtnA.isPressed();
    break;
  case BUTTON_MIDDLE:
    return M5.BtnB.isPressed();
    break;
  case BUTTON_RIGHT:
    return M5.BtnC.isPressed();
    break;
  default:
    return false;
    break;
  }
}

void BoardM5stackCore::enterSleepMode(int secs) {
  
  M5.setWakeupButton(GPIO_NUM_37);

  if (secs > 0)
  {
    syslog->println("Going to sleep for " + String(secs) + " seconds!");
  }
  else
  {
    syslog->println("Shutting down...");
  }

  syslog->flush();
  delay(100);

  M5.Axp.DeepSleep(secs * 1000000ULL);
}

void BoardM5stackCore::mainLoop() {

  Board320_240::mainLoop();
  M5.update();

}

#endif // BOARD_M5STACK_CORE
