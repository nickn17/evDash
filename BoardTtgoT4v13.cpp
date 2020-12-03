#ifndef BOARDTTGOT4V13_CPP
#define BOARDTTGOT4V13_CPP

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardTtgoT4v13.h"

/**
  Init board
*/
void BoardTtgoT4v13::initBoard() {

  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;
  //pinSpeaker = SPEAKER_PIN;
  pinBrightness = TFT_BL;

  Board320_240::initBoard();
}

#endif // BOARDTTGOT4V13_CPP
