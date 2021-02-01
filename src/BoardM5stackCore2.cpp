#ifdef BOARD_M5STACK_CORE2

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore2.h"

/**
  Init board
*/
void BoardM5stackCore2::initBoard() {

  invertDisplay = true;
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;

  M5.begin(true, true, false, true, kMBusModeInput);  // power from bus (power over COMMU for example)
  //M5.begin(true, true, false, true);                // power from USB connector
  M5.Axp.SetLed(false);

  Board320_240::initBoard();
}

bool BoardM5stackCore2::isButtonPressed(int button) {
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

void BoardM5stackCore2::enterSleepMode(int secs) {

  if (secs > 0)
  {
    syslog->println("Going to sleep for " + String(secs) + " seconds!");
    syslog->flush();
    delay(100);
    M5.Axp.DeepSleep(secs * 1000000ULL);
  }
  else
  {
    syslog->println("Shutting down...");
    syslog->flush();
    delay(100);
    M5.Axp.PowerOff();
  }
}

void BoardM5stackCore2::mainLoop() {

  Board320_240::mainLoop();
  M5.update();

}

#endif // BOARD_M5STACK_CORE2
