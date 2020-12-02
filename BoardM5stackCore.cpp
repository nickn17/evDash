#ifndef BOARDM5STACKCORE_CPP
#define BOARDM5STACKCORE_CPP

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore.h"

/**
  Init board
*/
void BoardM5stackCore::initBoard() {

  this->invertDisplay = true;
  this->pinButtonLeft = BUTTON_LEFT;
  this->pinButtonRight = BUTTON_RIGHT;
  this->pinButtonMiddle = BUTTON_MIDDLE;
  this->pinSpeaker = SPEAKER_PIN;
  this->pinBrightness = TFT_BL;

  Board320_240::initBoard();
  
}

#endif // BOARDM5STACKCORE_CPP
