#ifdef BOARD_TTGO_T4

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
  pinSdcardCs = SDCARD_CS;
  pinSdcardMosi = SDCARD_MOSI;
  pinSdcardMiso = SDCARD_MISO;
  pinSdcardSclk = SDCARD_SCLK;

  Board320_240::initBoard();
}

bool BoardTtgoT4v13::isButtonPressed(int button) {
  if(digitalRead(button) == HIGH) {
    return false;
  } else {
    return true;
  }
}

void BoardTtgoT4v13::wakeupBoard() {
  return;
}

void BoardTtgoT4v13::enterSleepMode(int secs) {

  if (secs > 0)
  {
    syslog->println("Going to sleep for " + String(secs) + " seconds!");
    esp_sleep_enable_timer_wakeup(secs * 1000000ULL);
  }
  else
  {
    syslog->println("Going to sleep for ever! (shutdown)");
  }

  syslog->flush();
  delay(100);

  esp_deep_sleep_start();
}

#endif // BOARD_TTGO_T4
