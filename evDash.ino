/*
  Project renamed from eNiroDashboard to evDash

  Serial console commands

   serviceUUID=xxx
   charTxUUID=xxx
   charRxUUID=xxx
   wifiSsid=xxx
   wifiPassword=xxx
   gprsApn=xxx
   remoteApiUrl=xxx
   remoteApiKey=xxx

  Required libraries
  - esp32 board support
  - tft_espi
  - ArduinoJson
  - TinyGPSPlus (m5stack GPS)

  SIM800L m5stack (https://github.com/kolaCZek)
  - SIM800L.h
*/

////////////////////////////////////////////////////////////
// SELECT HARDWARE
////////////////////////////////////////////////////////////

// Boards
//#define BOARD_TTGO_T4
#define BOARD_M5STACK_CORE

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

#include <SPI.h>
#include "BoardInterface.h"

#ifdef BOARD_TTGO_T4
#include "BoardTtgoT4v13.h"
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
#include "BoardM5stackCore.h"
#endif // BOARD_M5STACK_CORE

#include "config.h"
#include "LiveData.h"
#include "CarInterface.h"
#include "CarKiaEniro.h"
#include "CarHyundaiIoniq.h"
#include "CarRenaultZoe.h"
#include "CarKiaNiroPhev.h"
#include "CarKiaDebugObd2.h"
#include "CarBmwI3.h"

// Board, Car, Livedata (params, settings)
BoardInterface* board;
CarInterface* car;
LiveData* liveData;

/**
  Setup device
*/
void setup(void) {

  // Serial console
  syslog = new LogSerial();
  syslog->println("\nBooting device...");

  // Init settings/params
  liveData = new LiveData();
  liveData->initParams();

  // Turn off serial console
  if (liveData->settings.serialConsolePort == 255) {
    syslog->println("Serial console disabled...");
    syslog->flush();
    syslog->end();
  }

  // Init board
#ifdef BOARD_TTGO_T4
  board = new BoardTtgoT4v13();
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
  board = new BoardM5stackCore();
#endif // BOARD_M5STACK_CORE
  board->setLiveData(liveData);
  board->loadSettings();
  board->initBoard();

  // Init selected car interface
  switch (liveData->settings.carType) {
    case CAR_KIA_ENIRO_2020_39:
    case CAR_KIA_ENIRO_2020_64:
    case CAR_HYUNDAI_KONA_2020_39:
    case CAR_HYUNDAI_KONA_2020_64:
      car = new CarKiaEniro();
      break;
    case CAR_HYUNDAI_IONIQ_2018:
      car = new CarHyundaiIoniq();
      break;
    case CAR_KIA_NIRO_PHEV:
      car = new CarKiaNiroPhev();
      break;
    case CAR_RENAULT_ZOE:
      car = new CarRenaultZoe();
      break;
    case CAR_BMW_I3_2014:
      car = new CarBmwI3();
      break;
    default:
      car = new CarKiaDebugObd2();
  }

  car->setLiveData(liveData);
  car->activateCommandQueue();
  board->attachCar(car);

  // Redraw screen
  board->redrawScreen();

  // Finish board setup
  board->afterSetup();

  // End
  syslog->println("Device setup completed");
}

/**
  Main loop
*/
void loop() {
  board->mainLoop();
}
