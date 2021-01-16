/*
  evDash (older name eNiroDashboard)
  
  Serial console commands
   serviceUUID=xxx
   charTxUUID=xxx
   charRxUUID=xxx
   wifiSsid=xxx
   wifiPassword=xxx
   gprsApn=xxx
   remoteApiUrl=xxx
   remoteApiKey=xxx

  Required libraries, see INSTALLATION.rd
  SIM800L m5stack support by (https://github.com/kolaCZek)
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

  // Init settings/params
  liveData = new LiveData();
  liveData->initParams();

  // Serial console
  syslog = new LogSerial();

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

  // Turn on serial console
  if (liveData->settings.serialConsolePort != 255
    && liveData->settings.gpsHwSerialPort != liveData->settings.serialConsolePort
    && liveData->settings.gprsHwSerialPort != liveData->settings.serialConsolePort) {
    syslog->begin(115200);
  }

  syslog->println("\nBooting device...");

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

  // Finish board setup
  board->afterSetup();

  // Redraw screen
  board->redrawScreen();

  // End
  syslog->println("Device setup completed");
}

/**
  Main loop
*/
void loop() {
  board->mainLoop();
}
