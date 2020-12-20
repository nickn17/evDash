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
      Serial.println("onConnect");
    }

    // On BLE disconnect
    void onDisconnect(BLEClient* pclient) {
      //connected = false;
      Serial.println("onDisconnect");

      boardObj->displayMessage("BLE disconnected", "");
    }
};

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    // Called for each advertising BLE server.
    void onResult(BLEAdvertisedDevice advertisedDevice) {

      Serial.print("BLE advertised device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      Serial.println(advertisedDevice.getAddress().toString().c_str());

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
      //          Serial.print("ServiceDataUUID: ");
      //          Serial.println(advertisedDevice.getServiceDataUUID().toString().c_str());
      //        if (advertisedDevice.getServiceUUID().toString() != "<NULL>") {
      //          Serial.print("ServiceUUID: ");
      //          Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
      //        }
      //        }

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(liveDataObj->settings.serviceUUID)) &&
          (strcmp(advertisedDevice.getAddress().toString().c_str(), liveDataObj->settings.obdMacAddress) == 0)) {
        Serial.println("Stop scanning. Found my BLE device.");
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
        commObj->parseRow();
      liveDataObj->responseRow = "";
    } else {
      liveDataObj->responseRow += ch;
      if (liveDataObj->responseRow == ">") {
        if (liveDataObj->responseRowMerged != "") {
          Serial.print("merged:");
          Serial.println(liveDataObj->responseRowMerged);
          boardObj->parseRowMerged();
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

  Serial.println("BLE4 connectDevice");

  // Start BLE connection
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
}

/**
   Disconnect device
*/
void CommObd2Ble4::disconnectDevice() {

  Serial.println("COMM disconnectDevice");
  btStop();
}

/**
   Scan device list, from menu
*/
void CommObd2Ble4::scanDevices() {

  Serial.println("COMM scanDevices");
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
   Do connect BLE with server (OBD device)
*/
bool CommObd2Ble4::connectToServer(BLEAddress pAddress) {

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
   Main loop
*/
void CommObd2Ble4::mainLoop() {

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
      board->customConsoleCommand(response);
      response = response + ch;
      Serial.println(response);
      if (liveData->bleConnected) {
        liveData->pRemoteCharacteristicWrite->writeValue(response.c_str(), response.length());
      }
      response = "";
    } else {
      response = response + ch;
    }
  }

  // Can send next command from queue to OBD
  if (liveData->canSendNextAtCommand) {
    liveData->canSendNextAtCommand = false;
    doNextAtCommand();
  }

  if (board->scanDevices) {
    board->scanDevices = false;
    startBleScan();
  }

}

/**
  Do next AT command from queue
*/
bool CommObd2Ble4::doNextAtCommand() {

  // Restart loop with AT commands
  if (liveData->commandQueueIndex >= liveData->commandQueueCount) {
    liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
    board->redrawScreen();
    // log every queue loop (temp)
    liveData->params.sdcardCanNotify = true;
  }

  // Send AT command to obd
  liveData->commandRequest = liveData->commandQueue[liveData->commandQueueIndex];
  if (liveData->commandRequest.startsWith("ATSH")) {
    liveData->currentAtshRequest = liveData->commandRequest;
  }

  Serial.print(">>> ");
  Serial.println(liveData->commandRequest);
  String tmpStr = liveData->commandRequest + "\r";
  liveData->responseRowMerged = "";
  liveData->pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  liveData->commandQueueIndex++;

  return true;
}

/**
  Parse result from OBD, merge frames to sigle response
*/
bool CommObd2Ble4::parseRow() {

  // 1 frame data
  Serial.println(liveData->responseRow);

  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    liveData->responseRowMerged += liveData->responseRow.substring(2);
  }

  return true;
}
