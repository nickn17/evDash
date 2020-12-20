/*
   Project renamed from eNiroDashboard to evDash

  !! Supported BLE4 adapter is Vgate ICar Pro BLE4 version

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

//#define SIM800L_ENABLED

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

#include <sys/time.h>
#include "config.h"
#include "LiveData.h"
#include "CarInterface.h"
#include "CarKiaEniro.h"
#include "CarHyundaiIoniq.h"
#include "CarRenaultZoe.h"
#include "CarKiaNiroPhev.h"
#include "CarKiaDebugObd2.h"
#include "CarBmwI3.h"

#ifdef SIM800L_ENABLED
#include <ArduinoJson.h>
#include "SIM800L.h"

SIM800L* sim800l;
HardwareSerial SerialGPRS(2);
#endif //SIM800L_ENABLED

// Board, Car, Livedata (params, settings)
BoardInterface* board;
CarInterface* car;
LiveData* liveData;

/**
  SIM800L
*/
#ifdef SIM800L_ENABLED
bool sim800lSetup() {
  Serial.println("Setting SIM800L module");

  SerialGPRS.begin(9600);

  sim800l = new SIM800L((Stream *)&SerialGPRS, SIM800L_RST, 512 , 512);
  // SIM800L DebugMode:
  //sim800l = new SIM800L((Stream *)&SerialGPRS, SIM800L_RST, 512 , 512, (Stream *)&Serial);

  bool sim800l_ready = sim800l->isReady();
  for (uint8_t i = 0; i < 5 && !sim800l_ready; i++) {
    Serial.println("Problem to initialize SIM800L module, retry in 1 sec");
    delay(1000);
    sim800l_ready = sim800l->isReady();
  }

  if (!sim800l_ready) {
    Serial.println("Problem to initialize SIM800L module");
  } else {
    Serial.println("SIM800L module initialized");

    Serial.print("Setting GPRS APN to: ");
    Serial.println(liveData->settings.gprsApn);

    bool sim800l_gprs = sim800l->setupGPRS(liveData->settings.gprsApn);
    for (uint8_t i = 0; i < 5 && !sim800l_gprs; i++) {
      Serial.println("Problem to set GPRS APN, retry in 1 sec");
      delay(1000);
      sim800l_gprs = sim800l->setupGPRS(liveData->settings.gprsApn);
    }

    if (sim800l_gprs) {
      liveData->params.sim800l_enabled = true;
      Serial.println("GPRS APN set OK");
    } else {
      Serial.println("Problem to set GPRS APN");
    }
  }

  return true;
}

bool sendDataViaGPRS() {
  Serial.println("Sending data via GPRS");

  if (liveData->params.socPerc < 0) {
    Serial.println("No valid data, skipping data send");
    return false;
  }

  NetworkRegistration network = sim800l->getRegistrationStatus();
  if (network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    Serial.println("SIM800L module not connected to network, skipping data send");
    return false;
  }

  if (!sim800l->isConnectedGPRS()) {
    Serial.println("GPRS not connected... Connecting");
    bool connected = sim800l->connectGPRS();
    for (uint8_t i = 0; i < 5 && !connected; i++) {
      Serial.println("Problem to connect GPRS, retry in 1 sec");
      delay(1000);
      connected = sim800l->connectGPRS();
    }
    if (connected) {
      Serial.println("GPRS connected!");
    } else {
      Serial.println("GPRS not connected! Reseting SIM800L module!");
      sim800l->reset();
      sim800lSetup();

      return false;
    }
  }

  Serial.println("Start HTTP POST...");

  StaticJsonDocument<512> jsonData;

  jsonData["apikey"] = liveData->settings.remoteApiKey;
  jsonData["carType"] = liveData->settings.carType;
  jsonData["socPerc"] = liveData->params.socPerc;
  jsonData["sohPerc"] = liveData->params.sohPerc;
  jsonData["batPowerKw"] = liveData->params.batPowerKw;
  jsonData["batPowerAmp"] = liveData->params.batPowerAmp;
  jsonData["batVoltage"] = liveData->params.batVoltage;
  jsonData["auxVoltage"] = liveData->params.auxVoltage;
  jsonData["auxAmp"] = liveData->params.auxCurrentAmp;
  jsonData["batMinC"] = liveData->params.batMinC;
  jsonData["batMaxC"] = liveData->params.batMaxC;
  jsonData["batInletC"] = liveData->params.batInletC;
  jsonData["batFanStatus"] = liveData->params.batFanStatus;
  jsonData["speedKmh"] = liveData->params.speedKmh;
  jsonData["odoKm"] = liveData->params.odoKm;
  jsonData["cumulativeEnergyChargedKWh"] = liveData->params.cumulativeEnergyChargedKWh;
  jsonData["cumulativeEnergyDischargedKWh"] = liveData->params.cumulativeEnergyDischargedKWh;

  char payload[512];
  serializeJson(jsonData, payload);

  Serial.print("Sending payload: ");
  Serial.println(payload);

  Serial.print("Remote API server: ");
  Serial.println(liveData->settings.remoteApiUrl);

  uint16_t rc = sim800l->doPost(liveData->settings.remoteApiUrl, "application/json", payload, 10000, 10000);
  if (rc == 200) {
    Serial.println("HTTP POST successful");
  } else {
    // Failed...
    Serial.print("HTTP POST error: ");
    Serial.println(rc);
  }

  return true;
}
#endif //SIM800L_ENABLED

/**
  Setup device
*/
void setup(void) {

  // Serial console, init structures
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Booting device...");

  // Init settings/params, board library
  liveData = new LiveData();
  liveData->initParams();

#ifdef BOARD_TTGO_T4
  board = new BoardTtgoT4v13();
#endif // BOARD_TTGO_T4
#ifdef BOARD_M5STACK_CORE
  board = new BoardM5stackCore();
#endif // BOARD_M5STACK_CORE
  board->setLiveData(liveData);
  board->loadSettings();
  board->initBoard();

  // Car interface
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

  // Init time library
  struct timeval tv;
  tv.tv_sec = 1589011873;
  settimeofday(&tv, NULL);
  struct tm now;
  getLocalTime(&now, 0);
  liveData->params.chargingStartTime = liveData->params.currentTime = mktime(&now);

#ifdef SIM800L_ENABLED
  sim800lSetup();
#endif //SIM800L_ENABLED

  // Finish board setup
  board->afterSetup();

  // End
  Serial.println("Device setup completed");
}

/**
  Main loop
*/
void loop() {

#ifdef SIM800L_ENABLED
  if (liveData->params.lastDataSent + SIM800L_TIMER < liveData->params.currentTime && liveData->params.sim800l_enabled) {
    sendDataViaGPRS();
    liveData->params.lastDataSent = liveData->params.currentTime;
  }
#endif // SIM800L_ENABLED

  board->mainLoop();
}
