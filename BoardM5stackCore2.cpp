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

  M5.begin();

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

void BoardM5stackCore2::mainLoop() {

  Board320_240::mainLoop();
  M5.update();

}
