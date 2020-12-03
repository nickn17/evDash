#ifndef BOARDM5STACKCORE_CPP
#define BOARDM5STACKCORE_CPP

#include "BoardInterface.h"
#include "Board320_240.h"
#include "BoardM5stackCore.h"
#include <SPI.h>
#include <SD.h>

/**
  Init board
*/
void BoardM5stackCore::initBoard() {

  invertDisplay = true;
  pinButtonLeft = BUTTON_LEFT;
  pinButtonRight = BUTTON_RIGHT;
  pinButtonMiddle = BUTTON_MIDDLE;
  pinSpeaker = SPEAKER_PIN;
  pinBrightness = TFT_BL;

  Board320_240::initBoard();
}

void BoardM5stackCore::mainLoop() {

  Board320_240::mainLoop();

/*#define TFCARD_CS_PIN 4

  if (!SD.begin(TFCARD_CS_PIN, SPI, 40000000)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));  
  */
}


#endif // BOARDM5STACKCORE_CPP
