#include <BLEDevice.h>
#include "CommObd2Bt3.h"
#include "BoardInterface.h"
#include "LiveData.h"

/*CommObd2Bt3* commObj;
BoardInterface* boardObj;
LiveData* liveDataObj;
*/
/**
   Connect BT3 adapter
*/
void CommObd2Bt3::connectDevice() {

  /*commObj = this;
  liveDataObj = liveData;
  boardObj = board;
*/
  syslog->println("BLE4 connectDevice");

  // Start BLE connection
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we have detected a new device.
  // Specify that we want active scanning and start the scan to run for 10 seconds.
  syslog->println("Setup BLE scan");
/*  liveData->pBLEScan = BLEDevice::getScan();
  liveData->pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  liveData->pBLEScan->setInterval(1349);
  liveData->pBLEScan->setWindow(449);
  liveData->pBLEScan->setActiveScan(true);
*/
  // Skip BLE scan if middle button pressed
  if (strcmp(liveData->settings.obdMacAddress, "00:00:00:00:00:00") != 0 && !board->skipAdapterScan()) {
    syslog->println(liveData->settings.obdMacAddress);
    startBleScan();
  }
}

/**
   Disconnect device
*/
void CommObd2Bt3::disconnectDevice() {

  syslog->println("COMM disconnectDevice");
  btStop();
}

/**
   Scan device list, from menu
*/
void CommObd2Bt3::scanDevices() {

  syslog->println("COMM scanDevices");
  startBleScan();
}

///////////////////////////////////

/**
   Start ble scan
*/
void CommObd2Bt3::startBleScan() {

  liveData->foundMyBleDevice = NULL;
  liveData->scanningDeviceIndex = 0;
  board->displayMessage(" > Scanning BLE4 devices", "40sec.or hold middle&RST");

  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n",  ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());

  // Start scanning
  syslog->println("Scanning BLE devices...");
  syslog->print("Looking for ");
  syslog->println(liveData->settings.obdMacAddress);
  //BLEScanResults foundDevices = liveData->pBLEScan->start(40, false);
  syslog->print("Devices found: ");
  //syslog->println(foundDevices.getCount());
  syslog->println("Scan done!");
  //liveData->pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  /*char tmpStr1[20];
  sprintf(tmpStr1, "Found %d devices", foundDevices.getCount());
  board->displayMessage(" > Scanning BLE4 devices", tmpStr1);
*/
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
bool CommObd2Bt3::connectToServer(BLEAddress pAddress) {

  board->displayMessage(" > Connecting device", "");

  syslog->print("liveData->bleConnect ");
  syslog->println(pAddress.toString().c_str());
  board->displayMessage(" > Connecting device - init", pAddress.toString().c_str());

/*  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); //
  pSecurity->setCapability(ESP_IO_CAP_KBDISP);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
*/
  board->displayMessage(" > Connecting device", pAddress.toString().c_str());
//  liveData->pClient = BLEDevice::createClient();
  //liveData->pClient->setClientCallbacks(new MyClientCallback());
  //if (liveData->pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM) ) syslog->println("liveData->bleConnected");
  syslog->println(" - liveData->bleConnected to server");

  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n",  ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());

  // Remote service
  board->displayMessage(" > Connecting device", "Connecting service...");
  syslog->print("serviceUUID -");
  syslog->print(liveData->settings.serviceUUID);
  syslog->println("-");
/*  BLERemoteService* pRemoteService = liveData->pClient->getService(BLEUUID(liveData->settings.serviceUUID));
  if (pRemoteService == nullptr)
  {
    syslog->print("Failed to find our service UUID: ");
    syslog->println(liveData->settings.serviceUUID);
    board->displayMessage(" > Connecting device", "Unable to find service");
    return false;
  }
  */syslog->println(" - Found our service");
  syslog->printf("Total/free heap: %i/%i-%i, total/free PSRAM %i/%i bytes\n",  ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT), ESP.getPsramSize(), ESP.getFreePsram());

  // Get characteristics
  board->displayMessage(" > Connecting device", "Connecting TxUUID...");
  syslog->print("charTxUUID -");
  syslog->print(liveData->settings.charTxUUID);
  syslog->println("-");
  /*liveData->pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charTxUUID));
  if (liveData->pRemoteCharacteristic == nullptr) {
    syslog->print("Failed to find our characteristic UUID: ");
    syslog->println(liveData->settings.charTxUUID);//.toString().c_str());
    board->displayMessage(" > Connecting device", "Unable to find TxUUID");
    return false;
  }*/
  syslog->println(" - Found our characteristic");

  // Get characteristics
  board->displayMessage(" > Connecting device", "Connecting RxUUID...");
  syslog->print("charRxUUID -");
  syslog->print(liveData->settings.charRxUUID);
  syslog->println("-");
  /*liveData->pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(BLEUUID(liveData->settings.charRxUUID));
  if (liveData->pRemoteCharacteristicWrite == nullptr) {
    syslog->print("Failed to find our characteristic UUID: ");
    syslog->println(liveData->settings.charRxUUID);//.toString().c_str());
    board->displayMessage(" > Connecting device", "Unable to find RxUUID");
    return false;
  }*/
  syslog->println(" - Found our characteristic write");

  board->displayMessage(" > Connecting device", "Register callbacks...");
  // Read the value of the characteristic.
  /*if (liveData->pRemoteCharacteristic->canNotify()) {
    syslog->println(" - canNotify");
    if (liveData->pRemoteCharacteristic->canIndicate()) {
      syslog->println(" - canIndicate");
      const uint8_t indicationOn[] = {0x2, 0x0};
      //const uint8_t indicationOff[] = {0x0,0x0};
      //liveData->pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)indicationOn, 2, true);
      //liveData->pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notifyOff,2,true);
      //liveData->pRemoteCharacteristic->registerForNotify(notifyCallback, false);
      delay(200);
    }
  }*/

  board->displayMessage(" > Connecting device", "Done...");
  /*if (liveData->pRemoteCharacteristicWrite->canWrite()) {
    syslog->println(" - canWrite");
  }*/

  return true;
}

/**
   Main loop
*/
void CommObd2Bt3::mainLoop() {

  // Connect BLE device
  /*if (liveData->bleConnect == true && liveData->foundMyBleDevice != NULL) {
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
  }*/

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
void CommObd2Bt3::executeCommand(String cmd) {

  String tmpStr = cmd + "\r";
  if (liveData->commConnected) {
    //liveData->pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  }
}
