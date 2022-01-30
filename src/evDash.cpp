/*
  evDash (older name eNiroDashboard)

  Serial console commands
   serviceUUID=xxx
   charTxUUID=xxx
   charRxUUID=xxx
   wifiSsid=xxx
   wifiPassword=xxx
   wifiSsid2=xxx
   wifiPassword2=xxx
   gprsApn=xxx
   remoteApiUrl=xxx
   remoteApiKey=xxx
   abrpApiToken=xxx

  Required libraries, see INSTALLATION.rd
  SIM800L m5stack support by (https://github.com/kolaCZek)
*/

#include "Arduino.h"

#include "config.h"
#include "BoardInterface.h"

#ifdef BOARD_TTGO_T4
#include "BoardTtgoT4v13.h"
#endif // BOARD_TTGO_T4

#ifdef BOARD_M5STACK_CORE
#include "BoardM5stackCore.h"
#endif // BOARD_M5STACK_CORE

#ifdef BOARD_M5STACK_CORE2
#include "BoardM5stackCore2.h"
#endif // BOARD_M5STACK_CORE2

#include "LiveData.h"
#include "CarInterface.h"
#include "CarKiaEniro.h"
#include "CarHyundaiIoniq.h"
#include "CarHyundaiIoniq5.h"
#include "CarRenaultZoe.h"
#include "CarKiaDebugObd2.h"
#include "CarBmwI3.h"
#include "CarVWID3.h"
#include "CarPeugeotE208.h"

// Board, Car, Livedata (params, settings)
BoardInterface *board;
CarInterface *car;
LiveData *liveData;

/**
  Setup device
*/
void setup(void)
{
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

#ifdef BOARD_M5STACK_CORE2
  board = new BoardM5stackCore2();
#endif // BOARD_M5STACK_CORE2

  board->setLiveData(liveData);
  board->loadSettings();
  board->initBoard();

  // Turn on serial console
  if (liveData->settings.serialConsolePort != 255 && liveData->settings.gpsHwSerialPort != liveData->settings.serialConsolePort && liveData->settings.gprsHwSerialPort != liveData->settings.serialConsolePort)
  {
    syslog->begin(115200);
  }

  syslog->println("\nBooting device...");

  // Init selected car interface
  switch (liveData->settings.carType)
  {
  case CAR_KIA_ENIRO_2020_39:
  case CAR_KIA_ENIRO_2020_64:
  case CAR_HYUNDAI_KONA_2020_39:
  case CAR_HYUNDAI_KONA_2020_64:
  case CAR_KIA_ESOUL_2020_64:
    car = new CarKiaEniro();
    break;
  case CAR_HYUNDAI_IONIQ5_58:
  case CAR_HYUNDAI_IONIQ5_72:
  case CAR_HYUNDAI_IONIQ5_77:
    car = new CarHyundaiIoniq5();
    break;
  case CAR_HYUNDAI_IONIQ_2018:
    car = new CarHyundaiIoniq();
    break;
  case CAR_RENAULT_ZOE:
    car = new CarRenaultZoe();
    break;
  case CAR_BMW_I3_2014:
    car = new CarBmwI3();
    break;
  case CAR_VW_ID3_2021_45:
  case CAR_VW_ID3_2021_58:
  case CAR_VW_ID3_2021_77:
    car = new CarVWID3();
    break;
  case CAR_PEUGEOT_E208:
    car = new CarPeugeotE208();
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
  syslog->println("");
  syslog->println("▓█████ ██▒   █▓▓█████▄  ▄▄▄        ██████  ██░ ██ ");
  syslog->println("▓█   ▀▓██░   █▒▒██▀ ██▌▒████▄    ▒██    ▒ ▓██░ ██▒");
  syslog->println("▒███   ▓██  █▒░░██   █▌▒██  ▀█▄  ░ ▓██▄   ▒██▀▀██░");
  syslog->println("▒▓█  ▄  ▒██ █░░░▓█▄   ▌░██▄▄▄▄██   ▒   ██▒░▓█ ░██ ");
  syslog->println("░▒████▒  ▒▀█░  ░▒████▓  ▓█   ▓██▒▒██████▒▒░▓█▒░██▓");
  syslog->println("░░ ▒░ ░  ░ ▐░   ▒▒▓  ▒  ▒▒   ▓▒█░▒ ▒▓▒ ▒ ░ ▒ ░░▒░▒");
  syslog->println(" ░ ░  ░  ░ ░░   ░ ▒  ▒   ▒   ▒▒ ░░ ░▒  ░ ░ ▒ ░▒░ ░");
  syslog->println("   ░       ░░   ░ ░  ░   ░   ▒   ░  ░  ░   ░  ░░ ░");
  syslog->println("   ░  ░     ░     ░          ░  ░      ░   ░  ░  ░");
  syslog->println("           ░    ░                                 ");
  syslog->println("");
  syslog->println(".-[ HELP: Console commands ]-_.");
  syslog->println("reboot   ... reboot device");
  syslog->println("saveSettings   ... save current settings");
  syslog->println("debugLevel=n   [n = 0..3]  ... set debug level all, gps, comm, ...");
  syslog->println("wifiSsid=x     ... set primary AP ssid");
  syslog->println("wifiPassword=x     ... set primary AP password");
  syslog->println("wifiSsid2=x     ... set backup AP ssid (replace primary wifi automatically in 1-2 minutes)");
  syslog->println("wifiPassword2=x     ... set backup AP password");
  syslog->println("abrpApiToken=x     ... set abrp api token for live data");
  syslog->println("gprsApn=x     ... set gprs (sim800l) apn");
  syslog->println("remoteApiUrl=x     ... set remote api url");
  syslog->println("remoteApiKey=x     ... set remote api key");
  syslog->println("serviceUUID=x     ... set device uuid for obd2 ble adapter");
  syslog->println("charTxUUID=x     ... set tx uuid for obd2 ble adapter");
  syslog->println("charRxUUID=x     ... set rx uuid for obd2 ble adapter");
  syslog->println("time   ... print current time");
  syslog->println("setTime=2022-12-30 05:00:00  ... set current time");
  syslog->println("record=n   [n = 1..4]  ... record can response to buffer 1..4");
  syslog->println("compare      ... compare buffers");
  syslog->println("test     ... test handler");
  syslog->println("__________________________________________________");
}

/**
  Main loop
*/
void loop()
{
  board->mainLoop();
}
