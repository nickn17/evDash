#include <BLEDevice.h>
#include "CommObd2Ble4.h"
#include "BoardInterface.h"
#include "LiveData.h"

CommObd2Ble4* commObj;
BoardInterface* boardObj;
LiveData* liveDataObj;

/**
  BLE callbacks
*/
class MyClientCallback : public BLEClientCallbacks {

    // On BLE connect
    void onConnect(BLEClient* pclient) {
      syslog->println("onConnect");
    }

    // On BLE disconnect
    void onDisconnect(BLEClient* pclient) {
      liveDataObj->commConnected = false;
      syslog->println("onDisconnect");

      boardObj->displayMessage("BLE disconnected", "");
    }
};

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    // Called for each advertising BLE server.
    void onResult(BLEAdvertisedDevice advertisedDevice) {

      syslog->print("BLE advertised device found: ");
      syslog->println(advertisedDevice.toString().c_str());
      syslog->println(advertisedDevice.getAddress().toString().c_str());

      // Add to device list (max. 9 devices allowed yet)
      String tmpStr;

      if (liveDataObj->scanningDeviceIndex < 10) { // && advertisedDevice.haveServiceUUID()
        for (uint16_t i = 0; i < liveDataObj->menuItemsCount; ++i) {
          if (liveDataObj->menuItems[i].id == 10001 + liveDataObj->scanningDeviceIndex) {
            tmpStr = advertisedDevice.toString().c_str();
            tmpStr.replace("Name: ", "");
            tmpStr.replace("Address: ", "");
            tmpStr.toCharArray(liveDataObj->menuItems[i].title, 48);
            tmpStr = advertisedDevice.getAddress().toString().c_str();
            tmpStr.toCharArray(liveDataObj->menuItems[i].obdMacAddress, 18);
          }
        }
        liveDataObj->scanningDeviceIndex++;
      }

      //        if (advertisedDevice.getServiceDataUUID().toString() != "<NULL>") {
      //          syslog->print("ServiceDataUUID: ");
      //          syslog->println(advertisedDevice.getServiceDataUUID().toString().c_str());
      //        if (advertisedDevice.getServiceUUID().toString() != "<NULL>") {
      //          syslog->print("ServiceUUID: ");
      //          syslog->println(advertisedDevice.getServiceUUID().toString().c_str());
      //        }
      //        }

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(liveDataObj->settings.serviceUUID)) &&
          (strcmp(advertisedDevice.getAddress().toString().c_str(), liveDataObj->settings.obdMacAddress) == 0)) {
        syslog->println("Stop scanning. Found my BLE device.");
        BLEDevice::getScan()->stop();
        liveDataObj->foundMyBleDevice = new BLEAdvertisedDevice(advertisedDevice);
      }
    }
};

uint32_t PIN = 1234;

/**
  BLE Security
*/
class MySecurity : public BLESecurityCallbacks {

    uint32_t onPassKeyRequest() {
      syslog->printf("Pairing password: %d \r\n", PIN);
      return PIN;
    }

    void onPassKeyNotify(uint32_t pass_key) {
      syslog->printf("onPassKeyNotify\r\n");
    }

    bool onConfirmPIN(uint32_t pass_key) {
      syslog->printf("onConfirmPIN\r\n");
      return true;
    }

    bool onSecurityRequest() {
      syslog->printf("onSecurityRequest\r\n");
      return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
      if (auth_cmpl.success) {
        syslog->printf("onAuthenticationComplete\r\n");
      } else {
        syslog->println("Auth failure. Incorrect PIN?");
        liveDataObj->bleConnect = false;
      }
    }

};

/**
   Ble notification callback
*/
static void notifyCallback (BLERemoteCharacteristic * pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  char ch;

  // Parse multiframes to single response
  liveDataObj->responseRow = "";
  for (int i = 0; i <= length; i++) {
    ch = pData[i];
    if (ch == '\r'  || ch == '\n' || ch == '\0') {
      if (liveDataObj->responseRow != "")
        commObj->parseResponse();
      liveDataObj->responseRow = "";
    } else {
      liveDataObj->responseRow += ch;
      if (liveDataObj->responseRow == ">") {
        if (liveDataObj->responseRowMerged != "") {
          syslog->infoNolf(DEBUG_COMM, "merged:");
          syslog->info(DEBUG_COMM, liveDataObj->responseRowMerged);
          commObj->parseRowMerged();
        }
        liveDataObj->responseRowMerged = "";
        liveDataObj->canSendNextAtCommand = true;
      }
    }
  }
}

/**
   Connect ble4 adapter
*/
void CommObd2Ble4::connectDevice() {

  commObj = this;
  liveDataObj = liveData;
  boardObj = board;

  syslog->println("BLE4 connectDevice");

  // Start BLE connection
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we have detected a new device.
  // Specify that we want active scanning and start the scan to run for 10 seconds.
  syslog->println("Setup BLE scan");
  liveData->pBLEScan = BLEDevice::getScan();
  liveData->pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  liveData->pBLEScan->setInterval(1349);
  liveData->pBLEScan->setWindow(449);
  liveData->pBLEScan->setActiveScan(true);

  // Skip BLE scan if middle button pressed
  if (strcmp(liveData->settings.obdMacAddress, "00:00:00:00:00:00") != 0 && !board->skipAdapterScan()) {
    syslog->println(liveData->settings.obdMacAddress);
    startBleScan();
  }
}

/**
   Disconnect device
*/
void CommObd2Ble4::disconnectDevice() {

  syslog->println("COMM disconnectDevice");
  btStop();
}

/**
   Scan device list, from menu
*/
void CommObd2Ble4::scanDevices() {

  syslog->println("COMM scanDevices");
  startBleScan();
}

///////////////////////////////////

/**
   Start ble scan
*/
void CommObd2Ble4::startBleScan() {

  liveData->foundMyBleDevice = NULL;
  liveData->scanningDeviceIndex = 0;
  board->displayMessage(" > Scanning BLE4 devices", "40sec.or hold middle&RST");

  syslog->printf("FreeHeap: %i bytes\n", ESP.getFreeHeap());

  // Start scanning
  syslog->println("Scanning BLE devices...");
  syslog->print("Looking for ");
  syslog->println(liveData->settings.obdMacAddress);
  BLEScanResults foundDevices = liveData->pBLEScan->start(40, false);
  syslog->print("Devices found: ");
  syslog->println(foundDevices.getCount());
  syslog->println("Scan done!");
  liveData->pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  char tmpStr1[20];
  sprintf(tmpStr1, "Found %d devices", foundDevices.getCount());
  board->displayMessage(" > Scanning BLE4 devices", tmpStr1);

  // Scan devices from menu, show list of devices
 if (liveData->menuCurrent == 9999) {
    syslog->println("Display menu with devices");
    liveData->menuVisible = true;
    //liveData->menuCurrent = 9999;
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
   Do connect BLE with server (OBD device)
*/
bool CommObd2Ble4::connectToServer(BLEAddress pAddress) {

  board->displayMessage(" > Connecting device", "");

  syslog->print("liveData->bleConnect ");
  syslog->println(pAddress.toString().c_str());
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
  if (liveData->pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) syslog->println("liveData->bleConnected");
  syslog->println(" - liveData->bleConnected to server");

  syslog->printf("FreeHeap: %i bytes\n", ESP.getFreeHeap());

  // Remote service
  board->displayMessage(" > Connecting device", "Connecting service...");
  syslog->print("serviceUUID -");
  syslog->print(liveData->settings.serviceUUID);
  syslog->println("-");
  BLERemoteService* pRemoteService = liveData->pClient->getService(BLEUUID(liveData->settings.serviceUUID));
  if (pRemoteService == nullptr)
  {
    syslog->print("Failed to find our service UUID: ");
    syslog->println(liveData->settings.serviceUUID);
    board->displayMessage(" > Connecting device", "Unable to find service");
    return false;
  }
  syslog->println(" - Found our service");
  syslog->printf("FreeHeap: %i bytes\n", ESP.getFreeHeap());

  // Get characteristics
  board->displayMessage(" > Connecting device", "Connecting TxUUID...");
  syslog->print("charTxUUID -");
  syslog->print(liveData->settings.charTxUUID);
  syslog->println("-");
  liveData->pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charTxUUID));
  if (liveData->pRemoteCharacteristic == nullptr) {
    syslog->print("Failed to find our characteristic UUID: ");
    syslog->println(liveData->settings.charTxUUID);//.toString().c_str());
    board->displayMessage(" > Connecting device", "Unable to find TxUUID");
    return false;
  }
  syslog->println(" - Found our characteristic");

  // Get characteristics
  board->displayMessage(" > Connecting device", "Connecting RxUUID...");
  syslog->print("charRxUUID -");
  syslog->print(liveData->settings.charRxUUID);
  syslog->println("-");
  liveData->pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charRxUUID));
  if (liveData->pRemoteCharacteristicWrite == nullptr) {
    syslog->print("Failed to find our characteristic UUID: ");
    syslog->println(liveData->settings.charRxUUID);//.toString().c_str());
    board->displayMessage(" > Connecting device", "Unable to find RxUUID");
    return false;
  }
  syslog->println(" - Found our characteristic write");

  board->displayMessage(" > Connecting device", "Register callbacks...");
  // Read the value of the characteristic.
  if (liveData->pRemoteCharacteristic->canNotify()) {
    syslog->println(" - canNotify");
    if (liveData->pRemoteCharacteristic->canIndicate()) {
      syslog->println(" - canIndicate");
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
    syslog->println(" - canWrite");
  }

  return true;
}

/**
   Main loop
*/
void CommObd2Ble4::mainLoop() {

  // Connect BLE device
  if (liveData->bleConnect == true && liveData->foundMyBleDevice != NULL) {
    liveData->pServerAddress = new BLEAddress(liveData->settings.obdMacAddress);
    if (connectToServer(*liveData->pServerAddress)) {

      liveData->commConnected = true;
      liveData->bleConnect = false;

      syslog->println("We are now connected to the BLE device.");

      // Print message
      board->displayMessage(" > Processing init AT cmds", "");

      // Serve first command (ATZ)
      doNextQueueCommand();
      
    } else {
      syslog->println("We have failed to connect to the server; there is nothing more we will do.");
    }
  }

  // Parent declaration
  CommInterface::mainLoop();

  if (board->scanDevices) {
    board->scanDevices = false;
    startBleScan();
  }
}

/**
 * Send command
 */
void CommObd2Ble4::executeCommand(String cmd) {

  String tmpStr = cmd + "\r";
  if (liveData->commConnected) {
    liveData->pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  }
}
