/*
   Project renamed from eNiroDashboard to evDash

  !! working only with OBD BLE 4.0 adapters
  !! Supported adapter is  Vgate ICar Pro (must be BLE4.0 version)
  !! Not working with standard BLUETOOTH 3 adapters

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

  SIM800L m5stack (https://github.com/kolaCZek)
  - SIM800L.h, SoftwareSerial.h
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
#include <BLEDevice.h>
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
#include "CarKiaDebugObd2.h"

#ifdef SIM800L_ENABLED
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "SIM800L.h"

SIM800L* sim800l;
#endif //SIM800L_ENABLED

// PLEASE CHANGE THIS SETTING for your BLE4
uint32_t PIN = 1234;

// Temporary variables
char ch;
String line;

// Board, Car, Livedata (params, settings)
BoardInterface* board;
CarInterface* car;
LiveData* liveData;

/**
  Do next AT command from queue
*/
bool doNextAtCommand() {

  // Restart loop with AT commands
  if (liveData->commandQueueIndex >= liveData->commandQueueCount) {
    liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
    board->redrawScreen();
  }

  // Send AT command to obd
  liveData->commandRequest = liveData->commandQueue[liveData->commandQueueIndex];
  if (liveData->commandRequest.startsWith("ATSH")) {
    liveData->currentAtshRequest = liveData->commandRequest;
  }

  Serial.print(">>> ");
  Serial.println(liveData->commandRequest);
  String tmpStr = liveData->commandRequest + "\r";
  liveData->pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  liveData->commandQueueIndex++;

  return true;
}

/**
  Parse result from OBD, create single line liveData->responseRowMerged
*/
bool parseRow() {

  // Simple 1 line responses
  Serial.print("");
  Serial.println(liveData->responseRow);

  // Merge 0:xxxx 1:yyyy 2:zzzz to single xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    if (liveData->responseRow.charAt(0) == '0') {
      liveData->responseRowMerged = "";
    }
    liveData->responseRowMerged += liveData->responseRow.substring(2);
  }

  return true;
}

/**
  Parse merged row (after merge completed)
*/
bool parseRowMerged() {

  Serial.print("merged:");
  Serial.println(liveData->responseRowMerged);

  // Catch output for debug screen
  if (board->displayScreen == SCREEN_DEBUG) {
    if (board->debugCommandIndex == liveData->commandQueueIndex) {
      board->debugAtshRequest = liveData->currentAtshRequest;
      board->debugCommandRequest = liveData->commandRequest;
      board->debugLastString  = liveData->responseRowMerged;
    }
  }

  // Parse by selected car interface
  car->parseRowMerged();

  return true;
}

/**
  BLE callbacks
*/
class MyClientCallback : public BLEClientCallbacks {

    /**
      On BLE connect
    */
    void onConnect(BLEClient* pclient) {
      Serial.println("onConnect");
    }

    /**
      On BLE disconnect
    */
    void onDisconnect(BLEClient* pclient) {
      //connected = false;
      Serial.println("onDisconnect");
      board->displayMessage("BLE disconnected", "");
    }
};

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    /**
      Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {

      Serial.print("BLE advertised device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      Serial.println(advertisedDevice.getAddress().toString().c_str());

      // Add to device list (max. 9 devices allowed yet)
      String tmpStr;

      if (liveData->scanningDeviceIndex < 10/* && advertisedDevice.haveServiceUUID()*/) {
        for (uint16_t i = 0; i < liveData->menuItemsCount; ++i) {
          if (liveData->menuItems[i].id == 10001 + liveData->scanningDeviceIndex) {
            tmpStr = advertisedDevice.toString().c_str();
            tmpStr.replace("Name: ", "");
            tmpStr.replace("Address: ", "");
            tmpStr.toCharArray(liveData->menuItems[i].title, 48);
            tmpStr = advertisedDevice.getAddress().toString().c_str();
            tmpStr.toCharArray(liveData->menuItems[i].obdMacAddress, 18);
          }
        }
        liveData->scanningDeviceIndex++;
      }
      /*
        if (advertisedDevice.getServiceDataUUID().toString() != "<NULL>") {
        Serial.print("ServiceDataUUID: ");
        Serial.println(advertisedDevice.getServiceDataUUID().toString().c_str());
        if (advertisedDevice.getServiceUUID().toString() != "<NULL>") {
          Serial.print("ServiceUUID: ");
          Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
        }
        }*/

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(liveData->settings.serviceUUID)) &&
          (strcmp(advertisedDevice.getAddress().toString().c_str(), liveData->settings.obdMacAddress) == 0)) {
        Serial.println("Stop scanning. Found my BLE device.");
        BLEDevice::getScan()->stop();
        liveData->foundMyBleDevice = new BLEAdvertisedDevice(advertisedDevice);
      }
    }
};

/**
  BLE Security
*/
class MySecurity : public BLESecurityCallbacks {

    uint32_t onPassKeyRequest() {
      Serial.printf("Pairing password: %d \r\n", PIN);
      return PIN;
    }

    void onPassKeyNotify(uint32_t pass_key) {
      Serial.printf("onPassKeyNotify\r\n");
    }

    bool onConfirmPIN(uint32_t pass_key) {
      Serial.printf("onConfirmPIN\r\n");
      return true;
    }

    bool onSecurityRequest() {
      Serial.printf("onSecurityRequest\r\n");
      return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
      if (auth_cmpl.success) {
        Serial.printf("onAuthenticationComplete\r\n");
      } else {
        Serial.println("Auth failure. Incorrect PIN?");
        liveData->bleConnect = false;
      }
    }
};

/**
   Ble notification callback
*/
static void notifyCallback (BLERemoteCharacteristic * pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  char ch;

  // Parse multi line response to single lines
  liveData->responseRow = "";
  for (int i = 0; i <= length; i++) {
    ch = pData[i];
    if (ch == '\r'  || ch == '\n' || ch == '\0') {
      if (liveData->responseRow != "")
        parseRow();
      liveData->responseRow = "";
    } else {
      liveData->responseRow += ch;
      if (liveData->responseRow == ">") {
        if (liveData->responseRowMerged != "") {
          parseRowMerged();
        }
        liveData->responseRowMerged = "";
        liveData->canSendNextAtCommand = true;
      }
    }
  }
}

/**
   Do connect BLE with server (OBD device)
*/
bool connectToServer(BLEAddress pAddress) {

  board->displayMessage(" > Connecting device", "");

  Serial.print("liveData->bleConnect ");
  Serial.println(pAddress.toString().c_str());
  board->displayMessage(" > Connecting device - init", pAddress.toString().c_str());

  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); //
  pSecurity->setCapability(ESP_IO_CAP_KBDISP);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  board->displayMessage(" > Connecting device", pAddress.toString().c_str());
  liveData->pClient = BLEDevice::createClient();
  liveData->pClient->setClientCallbacks(new MyClientCallback());
  if (liveData->pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) Serial.println("liveData->bleConnected");
  Serial.println(" - liveData->bleConnected to server");

  // Remote service
  board->displayMessage(" > Connecting device", "Connecting service...");
  BLERemoteService* pRemoteService = liveData->pClient->getService(BLEUUID(liveData->settings.serviceUUID));
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(liveData->settings.serviceUUID);
    board->displayMessage(" > Connecting device", "Unable to find service");
    return false;
  }
  Serial.println(" - Found our service");

  // Get characteristics
  board->displayMessage(" > Connecting device", "Connecting TxUUID...");
  liveData->pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charTxUUID));
  if (liveData->pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(liveData->settings.charTxUUID);//.toString().c_str());
    board->displayMessage(" > Connecting device", "Unable to find TxUUID");
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Get characteristics
  board->displayMessage(" > Connecting device", "Connecting RxUUID...");
  liveData->pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charRxUUID));
  if (liveData->pRemoteCharacteristicWrite == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(liveData->settings.charRxUUID);//.toString().c_str());
    board->displayMessage(" > Connecting device", "Unable to find RxUUID");
    return false;
  }
  Serial.println(" - Found our characteristic write");

  board->displayMessage(" > Connecting device", "Register callbacks...");
  // Read the value of the characteristic.
  if (liveData->pRemoteCharacteristic->canNotify()) {
    Serial.println(" - canNotify");
    //liveData->pRemoteCharacteristic->registerForNotify(notifyCallback);
    if (liveData->pRemoteCharacteristic->canIndicate()) {
      Serial.println(" - canIndicate");
      const uint8_t indicationOn[] = {0x2, 0x0};
      //const uint8_t indicationOff[] = {0x0,0x0};
      liveData->pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)indicationOn, 2, true);
      //liveData->pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notifyOff,2,true);
      liveData->pRemoteCharacteristic->registerForNotify(notifyCallback, false);
      delay(200);
    }
  }

  board->displayMessage(" > Connecting device", "Done...");
  if (liveData->pRemoteCharacteristicWrite->canWrite()) {
    Serial.println(" - canWrite");
  }

  return true;
}

/**
   Start ble scan
*/
void startBleScan() {

  liveData->foundMyBleDevice = NULL;
  liveData->scanningDeviceIndex = 0;
  board->displayMessage(" > Scanning BLE4 devices", "40sec.or hold middle&RST");

  // Start scanning
  Serial.println("Scanning BLE devices...");
  Serial.print("Looking for ");
  Serial.println(liveData->settings.obdMacAddress);
  BLEScanResults foundDevices = liveData->pBLEScan->start(40, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  liveData->pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  char tmpStr1[20];
  sprintf(tmpStr1, "Found %d devices", foundDevices.getCount());
  board->displayMessage(" > Scanning BLE4 devices", tmpStr1);

  // Scan devices from menu, show list of devices
  if (liveData->menuItemSelected == 2) {
    Serial.println("Display menu with devices");
    liveData->menuVisible = true;
    liveData->menuCurrent = 9999;
    liveData->menuItemSelected = 0;
    board->showMenu();
  } else {
    // Redraw screen
    if (liveData->foundMyBleDevice == NULL) {
      board->displayMessage("Device not found", "Middle button - menu");
    } else {
      board->redrawScreen();
    }
  }
}

/**
  SIM800L
*/
#ifdef SIM800L_ENABLED
bool sim800lSetup() {
  Serial.println("Setting SIM800L module");
  SoftwareSerial* serial = new SoftwareSerial(SIM800L_RX, SIM800L_TX);
  serial->begin(9600);
  sim800l = new SIM800L((Stream *)serial, SIM800L_RST, 512 , 512);

  // SIM800L DebugMode:
  //sim800l = new SIM800L((Stream *)serial, SIM800L_RST, 512 , 512, (Stream *)&Serial);

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

  NetworkRegistration network = sim800l->getRegistrationStatus();
  if (network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    Serial.println("SIM800L module not connected to network, skipping data send");
    return false;
  }

  if(!sim800l->isConnectedGPRS()) {
    Serial.println("GPRS not connected... Connecting");
    bool connected = sim800l->connectGPRS();
    for (uint8_t i = 0; i < 5 && !connected; i++) {
      Serial.println("Problem to connect GPRS, retry in 1 sec");
      delay(1000);
      connected = sim800l->connectGPRS();
    }
    if(connected) {
      Serial.println("GPRS connected!");
    } else {
      Serial.println("GPRS not connected! Reseting SIM800L module!");
      sim800l->reset();
      sim800lSetup();
  
      return false;
    }
  }

  Serial.println("Start HTTP POST...");

  StaticJsonDocument<450> jsonData;

  jsonData["apikey"] = liveData->settings.remoteApiKey;
  jsonData["carType"] = liveData->settings.carType;
  jsonData["socPerc"] = liveData->params.socPerc;
  jsonData["sohPerc"] = liveData->params.sohPerc;
  jsonData["batPowerKw"] = liveData->params.batPowerKw;
  jsonData["batPowerAmp"] = liveData->params.batPowerAmp;
  jsonData["batVoltage"] = liveData->params.batVoltage;
  jsonData["auxVoltage"] = liveData->params.auxVoltage;
  jsonData["batMinC"] = liveData->params.batMinC;
  jsonData["batMaxC"] = liveData->params.batMaxC;
  jsonData["batInletC"] = liveData->params.batInletC;
  jsonData["batFanStatus"] = liveData->params.batFanStatus;
  jsonData["cumulativeEnergyChargedKWh"] = liveData->params.cumulativeEnergyChargedKWh;
  jsonData["cumulativeEnergyDischargedKWh"] = liveData->params.cumulativeEnergyDischargedKWh;

  char payload[450];
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
  line = "";
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
  if (liveData->settings.carType == CAR_KIA_ENIRO_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64 ||
      liveData->settings.carType == CAR_KIA_ENIRO_2020_39 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39) {
    car = new CarKiaEniro();
  } else if (liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018) {
    car = new CarHyundaiIoniq();
  } else if (liveData->settings.carType == CAR_RENAULT_ZOE) {
    car = new CarRenaultZoe();
  } else {
    // if (liveData->settings.carType == CAR_DEBUG_OBD2_KIA)
    car = new CarKiaDebugObd2();
  }
  car->setLiveData(liveData);
  car->activateCommandQueue();
  board->attachCar(car);
  board->debugCommandIndex = liveData->commandQueueLoopFrom;

  // Redraw screen
  board->redrawScreen();

  // Init time library
  struct timeval tv;
  tv.tv_sec = 1589011873;
  settimeofday(&tv, NULL);
  struct tm now;
  getLocalTime(&now, 0);
  liveData->params.chargingStartTime = liveData->params.currentTime = mktime(&now);

  // Start BLE connection
  Serial.println("Start BLE with PIN auth");
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we have detected a new device.
  // Specify that we want active scanning and start the scan to run for 10 seconds.
  Serial.println("Setup BLE scan");
  liveData->pBLEScan = BLEDevice::getScan();
  liveData->pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  liveData->pBLEScan->setInterval(1349);
  liveData->pBLEScan->setWindow(449);
  liveData->pBLEScan->setActiveScan(true);

  // Skip BLE scan if middle button pressed
  if (strcmp(liveData->settings.obdMacAddress, "00:00:00:00:00:00") != 0 && !board->skipAdapterScan()) {
    Serial.println(liveData->settings.obdMacAddress);
    startBleScan();
  }

#ifdef SIM800L_ENABLED
  sim800lSetup();
#endif //SIM800L_ENABLED

  // Hold right button
  board->afterSetup();

  // End
  Serial.println("Device setup completed");
}

/**
  Loop
*/
void loop() {

  // Connect BLE device
  if (liveData->bleConnect == true && liveData->foundMyBleDevice != NULL) {
    liveData->pServerAddress = new BLEAddress(liveData->settings.obdMacAddress);
    if (connectToServer(*liveData->pServerAddress)) {

      liveData->bleConnected = true;
      liveData->bleConnect = false;

      Serial.println("We are now connected to the BLE device.");

      // Print message
      board->displayMessage(" > Processing init AT cmds", "");

      // Serve first command (ATZ)
      doNextAtCommand();
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
  }

  // Send command from TTY to OBD2
  if (Serial.available()) {
    ch = Serial.read();
    if (ch == '\r' || ch == '\n') {
      board->customConsoleCommand(line);
      line = line + ch;
      Serial.println(line);
      if (liveData->bleConnected) {
        liveData->pRemoteCharacteristicWrite->writeValue(line.c_str(), line.length());
      }
      line = "";
    } else {
      line = line + ch;
    }

    // Can send next command from queue to OBD
    if (liveData->canSendNextAtCommand) {
      liveData->canSendNextAtCommand = false;
      doNextAtCommand();
    }
  }

#ifdef SIM800L_ENABLED
  if (liveData->params.lastDataSent + SIM800L_TIMER < liveData->params.currentTime && liveData->params.sim800l_enabled) {
    sendDataViaGPRS();
    liveData->params.lastDataSent = liveData->params.currentTime;
  }
#endif // SIM800L_ENABLED

  board->mainLoop();

  // currentTime & 1ms delay
  struct tm now;
  getLocalTime(&now, 0);
  liveData->params.currentTime = mktime(&now);
  // Shutdown when car is off
  if (liveData->params.automaticShutdownTimer != 0 && liveData->params.currentTime - liveData->params.automaticShutdownTimer > 5)
    board->shutdownDevice();
  if (board->scanDevices) {
    board->scanDevices = false;
    startBleScan();
  }
}
