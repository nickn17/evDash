/**
 * Includes header files for different boards, cars, livedata,
 * and various components used by the evDash application.
 * Sets up the board, car interface, livedata, and logging.
 */

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
   remoteApiUrl=xxx
   remoteApiKey=xxx
   abrpApiToken=xxx

  Required libraries, see INSTALLATION.rd
*/

#define USE_M5_FONT_CREATOR

#include "Arduino.h"

#include "config.h"
#include "BoardInterface.h"

#ifdef BOARD_M5STACK_CORE2
#include "BoardM5stackCore2.h"
#endif // BOARD_M5STACK_CORE2

#ifdef BOARD_M5STACK_CORES3
#include "BoardM5stackCoreS3.h"
#endif // BOARD_M5STACK_CORES3

#include "LogSerial.h"
#include "LiveData.h"
#include "CarInterface.h"
#include "CarKiaEniro.h"
#include "CarHyundaiIoniq.h"
#include "CarHyundaiIoniqPHEV.h"
#include "CarHyundaiEgmp.h"
#include "CarRenaultZoe.h"
#include "CarBmwI3.h"
#include "CarVWID3.h"
#include "CarPeugeotE208.h"

// Board, Car, Livedata (params, settings)
BoardInterface *board;
CarInterface *car;
LiveData *liveData;

/**
 * Setup function that initializes the board, car interface, live data,
 * and logging. Also prints some introductory text.
 */
void setup(void)
{
  // Init settings/params
  liveData = new LiveData();
  liveData->initParams();

  // Serial console
  syslog = new LogSerial();

#ifdef BOARD_M5STACK_CORE2
  board = new BoardM5stackCore2();
#endif // BOARD_M5STACK_CORE2

#ifdef BOARD_M5STACK_CORES3
  board = new BoardM5stackCoreS3();
#endif // BOARD_M5STACK_CORES3

  board->setLiveData(liveData);
  board->loadSettings();
  board->initBoard();

  // Turn on serial console
  if (liveData->settings.serialConsolePort != 255 && liveData->settings.gpsHwSerialPort != liveData->settings.serialConsolePort)
  {
    syslog->begin(115200);
  }

  syslog->println("\nBooting device...");
  // board->resetSettings();

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
  case CAR_HYUNDAI_IONIQ6_77:
  case CAR_KIA_EV6_58:
  case CAR_KIA_EV6_77:
  case CAR_KIA_EV9_100:
    car = new CarHyundaiEgmp();
    break;
  case CAR_HYUNDAI_IONIQ_2018:
    car = new CarHyundaiIoniq();
    break;
  case CAR_HYUNDAI_IONIQ_PHEV:
    car = new CarHyundaiIoniqPHEV();
    break;
  case CAR_RENAULT_ZOE:
    car = new CarRenaultZoe();
    break;
  case CAR_BMW_I3_2014:
    car = new CarBmwI3();
    break;
  case CAR_AUDI_Q4_35:
  case CAR_AUDI_Q4_40:
  case CAR_AUDI_Q4_45:
  case CAR_AUDI_Q4_50:
  case CAR_SKODA_ENYAQ_55:
  case CAR_SKODA_ENYAQ_62:
  case CAR_SKODA_ENYAQ_82:
  case CAR_VW_ID3_2021_45:
  case CAR_VW_ID3_2021_58:
  case CAR_VW_ID3_2021_77:
  case CAR_VW_ID4_2021_45:
  case CAR_VW_ID4_2021_58:
  case CAR_VW_ID4_2021_77:
    car = new CarVWID3();
    break;
  case CAR_PEUGEOT_E208:
    car = new CarPeugeotE208();
    break;
  default:
    car = new CarKiaEniro();
  }

  car->setLiveData(liveData);
  car->activateCommandQueue();
  board->attachCar(car);

  // Finish board setup
  board->afterSetup();
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
  syslog->println("shutdown ... shutdown device");
  syslog->println("saveSettings   ... save current settings");
  syslog->println("factoryReset   ... reset settings");
  syslog->println("ipconfig   ... print network settings");
  syslog->println("ABRP_debug   ... print ABRP user token");
  syslog->println("debugLevel=n   [n = 0..4]  ... set debug level all, gps, comm, ...");
  syslog->println("wifiSsid=x     ... set primary AP ssid");
  syslog->println("wifiPassword=x     ... set primary AP password");
  syslog->println("wifiSsid2=x     ... set backup AP ssid (replace primary wifi automatically in 1-2 minutes)");
  syslog->println("wifiPassword2=x     ... set backup AP password");
  syslog->println("abrpApiToken=x     ... set abrp api token for live data");
  syslog->println("remoteApiUrl=x     ... set remote api url");
  syslog->println("remoteApiKey=x     ... set remote api key");
  syslog->println("mqttServer=x     ... set Mqtt server");
  syslog->println("mqttId=x     ... set Mqtt id");
  syslog->println("mqttUsername=x     ... set Mqtt username");
  syslog->println("mqttPassword=x     ... set Mqtt password");
  syslog->println("mqttPubTopic=x     ... set Mqtt publish topic");
  syslog->println("serviceUUID=x     ... set device uuid for obd2 ble adapter");
  syslog->println("charTxUUID=x     ... set tx uuid for obd2 ble adapter");
  syslog->println("charRxUUID=x     ... set rx uuid for obd2 ble adapter");
  syslog->println("obd2ip=x     ... set ip for obd2 wifi adapter");
  syslog->println("obd2port=x     ... set port for obd2 wifi adapter");
  syslog->println("time   ... print current time");
  syslog->println("ntpSync   ... sync Time with de.pool.ntp.org");
  syslog->println("setTime=2022-12-30 05:00:00  ... set current time");
  syslog->println("record=n   [n = 1..4]  ... record can response to buffer 1..4");
  syslog->println("compare      ... compare buffers");
  syslog->println("test     ... test handler");
  syslog->println("__________________________________________________");
}

/**
 * Main program loop that calls the board's mainLoop() method.
 * This handles running the main program logic in a loop.
 */
void loop()
{
  board->mainLoop();
}
