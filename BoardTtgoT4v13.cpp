#ifndef BOARDTTGOT4V13_CPP
#define BOARDTTGOT4V13_CPP

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardTtgoT4v13.h"

/**
  Init board
*/
void BoardTtgoT4v13::initBoard() {

  this->pinButtonLeft = BUTTON_LEFT;
  this->pinButtonRight = BUTTON_RIGHT;
  this->pinButtonMiddle = BUTTON_MIDDLE;
  //this->pinSpeaker = SPEAKER_PIN;
  this->pinBrightness = TFT_BL;

  Board320_240::initBoard();
}

#endif // BOARDTTGOT4V13_CPP
