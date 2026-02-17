#include <BLEDevice.h>
#include "CommObd2Ble4.h"
#include "BoardInterface.h"
#include "LiveData.h"

CommObd2Ble4 *commObj;
BoardInterface *boardObj;
LiveData *liveDataObj;

namespace
{
  constexpr uint32_t kBleConnectRetryBaseMs = 3000;
  constexpr uint32_t kBleConnectRetryMaxMs = 15000;

  bool hasConfiguredBleMac(const char *value)
  {
    return value != nullptr &&
           value[0] != '\0' &&
           strcmp(value, "00:00:00:00:00:00") != 0 &&
           strcmp(value, "not_set") != 0 &&
           strcmp(value, "empty") != 0;
  }

  uint32_t calcConnectRetryDelayMs(uint8_t failCount)
  {
    uint32_t backoffStep = failCount;
    if (backoffStep > 4)
      backoffStep = 4;

    uint32_t delayMs = kBleConnectRetryBaseMs * (backoffStep + 1);
    if (delayMs > kBleConnectRetryMaxMs)
      delayMs = kBleConnectRetryMaxMs;
    return delayMs;
  }
} // namespace

/**
  BLE callbacks
*/
class MyClientCallback : public BLEClientCallbacks
{

  // On BLE connect
  void onConnect(BLEClient *pclient)
  {
    syslog->println("onConnect");
    liveDataObj->commConnected = true;
    boardObj->displayMessage("BLE connected", "");
  }

  // On BLE disconnect
  void onDisconnect(BLEClient *pclient)
  {
    liveDataObj->commConnected = false;
    syslog->println("onDisconnect");

    boardObj->displayMessage("BLE disconnected", "");
  }
};

/**
   Scan for BLE servers and find the selected adapter by MAC address.
*/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{

  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {

    syslog->print("BLE advertised device found: ");
    syslog->println(advertisedDevice.toString().c_str());
    syslog->println(advertisedDevice.getAddress().toString().c_str());

    // Add to the device list (maximum of 9 devices allowed for now)
    String tmpStr;

    if (liveDataObj->scanningDeviceIndex < 10)
    { // Check if the device name and manufacturer data are empty, skip adding to the list
      for (uint16_t i = 0; i < liveDataObj->menuItemsCount; ++i)
      {
        if (liveDataObj->menuItems[i].id == 10001 + liveDataObj->scanningDeviceIndex)
        {
          String deviceName = advertisedDevice.getName().c_str();
          String manufacturerData = advertisedDevice.getManufacturerData().c_str();

          if (deviceName.isEmpty() && manufacturerData.isEmpty())
          {
            syslog->println("Skipping device with empty name and manufacturer data.");
            continue;
          }

          // Populate the string with name, manufacturer, and MAC address
          tmpStr = deviceName;
          if (!manufacturerData.isEmpty())
          {
            tmpStr += ", ";
            tmpStr += manufacturerData;
          }
          tmpStr += ", ";
          tmpStr += advertisedDevice.getAddress().toString().c_str();

          // Save to menuItems
          tmpStr.toCharArray(liveDataObj->menuItems[i].title, sizeof(liveDataObj->menuItems[i].title));
          tmpStr = advertisedDevice.getAddress().toString().c_str();
          tmpStr.toCharArray(liveDataObj->menuItems[i].obdMacAddress, sizeof(liveDataObj->menuItems[i].obdMacAddress));
        }
      }
      liveDataObj->scanningDeviceIndex++;
    }

    if (strcmp(advertisedDevice.getAddress().toString().c_str(), liveDataObj->settings.obdMacAddress) == 0)
    {
      if (advertisedDevice.haveServiceUUID() &&
          advertisedDevice.isAdvertisingService(BLEUUID(liveDataObj->settings.serviceUUID)))
      {
        syslog->println("Stop scanning. Found target MAC + matching service UUID.");
      }
      else
      {
        // Some adapters do not advertise a service UUID during scan or use a different one.
        syslog->println("Stop scanning. Found target MAC (service UUID differs/missing, using auto-detect).");
      }
      BLEDevice::getScan()->stop();
      liveDataObj->foundMyBleDevice = new BLEAdvertisedDevice(advertisedDevice);
    }
  }
};

uint32_t PIN = 1234;

/**
  BLE Security
*/
class MySecurity : public BLESecurityCallbacks
{

  uint32_t onPassKeyRequest()
  {
    syslog->printf("Pairing password: %d \r\n", PIN);
    return PIN;
  }

  void onPassKeyNotify(uint32_t pass_key)
  {
    syslog->printf("onPassKeyNotify\r\n");
  }

  bool onConfirmPIN(uint32_t pass_key)
  {
    syslog->printf("onConfirmPIN\r\n");
    return true;
  }

  bool onSecurityRequest()
  {
    syslog->printf("onSecurityRequest\r\n");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
  {
    if (auth_cmpl.success)
    {
      syslog->printf("onAuthenticationComplete\r\n");
    }
    else
    {
      syslog->println("Auth failure. Incorrect PIN?");
      liveDataObj->obd2ready = false;
    }
  }
};

/**
   Ble notification callback
*/
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

  char ch;

  // Parse multiframes to single response
  liveDataObj->responseRow = "";
  for (int i = 0; i <= length; i++)
  {
    ch = pData[i];
    if (ch == '\r' || ch == '\n' || ch == '\0')
    {
      if (liveDataObj->responseRow != "")
        commObj->parseResponse();
      liveDataObj->responseRow = "";
    }
    else
    {
      liveDataObj->responseRow += ch;
      if (liveDataObj->responseRow == ">")
      {
        if (liveDataObj->responseRowMerged != "")
        {
          syslog->infoNolf(DEBUG_COMM, "merged: ");
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
void CommObd2Ble4::connectDevice()
{

  commObj = this;
  liveDataObj = liveData;
  boardObj = board;

  syslog->println("BLE4 connectDevice");
  connectFailCount = 0;
  nextConnectRetryMs = 0;
  liveData->commConnected = false;
  liveData->obd2ready = true;
  liveData->pRemoteCharacteristic = nullptr;
  liveData->pRemoteCharacteristicWrite = nullptr;
  liveData->foundMyBleDevice = nullptr;

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

  // Avoid 10-second blocking scan during boot.
  // Connection is attempted directly by configured MAC in mainLoop().
  if (hasConfiguredBleMac(liveData->settings.obdMacAddress) && !board->skipAdapterScan())
  {
    syslog->println("Boot BLE scan skipped (non-blocking startup connect by MAC).");
  }
}

/**
   Disconnect device
*/
void CommObd2Ble4::disconnectDevice()
{

  syslog->println("COMM disconnectDevice");
  btStop();
}

/**
   Scan device list, from menu
*/
void CommObd2Ble4::scanDevices()
{

  syslog->println("COMM scanDevices");
  startBleScan();
}

///////////////////////////////////

/**
   Start ble scan
*/
void CommObd2Ble4::startBleScan()
{

  liveData->foundMyBleDevice = NULL;
  liveData->scanningDeviceIndex = 0;
  board->displayMessage(" > Scanning BLE4 devices", "10sec.or hold middle&RST");

  syslog->printf("Total/free heap: %i/%i-%i\n", ESP.getHeapSize(), ESP.getFreeHeap(), heap_caps_get_free_size(MALLOC_CAP_8BIT));

  // Start scanning
  syslog->println("Scanning BLE devices...");
  syslog->print("Looking for ");
  syslog->println(liveData->settings.obdMacAddress);
  BLEScanResults foundDevices = liveData->pBLEScan->start(10, false);
  syslog->print("Devices found: ");
  syslog->println(foundDevices.getCount());
  syslog->println("Scan done!");
  liveData->pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  char tmpStr1[20];
  sprintf(tmpStr1, "Found %d devices", foundDevices.getCount());
  board->displayMessage(" > Scanning BLE4 devices", tmpStr1);

  // Scan devices from menu, show list of devices
  if (liveData->menuCurrent == 9999)
  {
    syslog->println("Display menu with devices");
    liveData->menuVisible = true;
    // liveData->menuCurrent = 9999;
    liveData->menuItemSelected = 0;
    board->showMenu();
  }
  else
  {
    // Redraw screen
    if (liveData->foundMyBleDevice == NULL)
    {
      board->displayMessage("Device not found", "Middle button - menu");
    }
    else
    {
      board->redrawScreen();
    }
  }
}

/**
   Connect to BLE device and automatically detect service and characteristics
*/
bool CommObd2Ble4::connectToServer(BLEAddress pAddress)
{
  board->displayMessage(" > Connecting device", "");

  syslog->print("Connecting to device: ");
  syslog->println(pAddress.toString().c_str());
  board->displayMessage(" > Connecting device - init", pAddress.toString().c_str());

  // Set BLE encryption and security
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setCapability(ESP_IO_CAP_KBDISP);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  // Create BLE client and set callbacks
  board->displayMessage(" > Connecting device", pAddress.toString().c_str());
  if (liveData->pClient == nullptr)
  {
    liveData->pClient = BLEDevice::createClient();
    if (liveData->pClient == nullptr)
    {
      syslog->println("Failed to create BLE client.");
      return false;
    }
    liveData->pClient->setClientCallbacks(new MyClientCallback());
  }
  else if (liveData->pClient->isConnected())
  {
    liveData->pClient->disconnect();
  }

  // Attempt to connect to the BLE device.
  // Most adapters use random address type, but some use public type.
  bool connected = liveData->pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM);
  if (!connected)
  {
    syslog->println("Connect with RANDOM address type failed. Trying PUBLIC...");
    connected = liveData->pClient->connect(pAddress, BLE_ADDR_TYPE_PUBLIC);
  }

  if (!connected)
  {
    syslog->println("Failed to connect to BLE device.");
    board->displayMessage("Connection failed", "Will retry...");
    return false;
  }
  syslog->println("Successfully connected to BLE device.");
  board->displayMessage("> Successfully connected", pAddress.toString().c_str());

  // Discover all services
  std::map<std::string, BLERemoteService *> *services = liveData->pClient->getServices();
  if (services == nullptr || services->empty())
  {
    syslog->println("No services found.");
    liveData->pClient->disconnect();
    return false;
  }

  // Loop through services to find characteristics
  liveData->pRemoteCharacteristic = nullptr;
  liveData->pRemoteCharacteristicWrite = nullptr;

  for (auto const &entry : *services)
  {
    BLERemoteService *pRemoteService = entry.second;
    syslog->print("Detected service UUID: ");
    syslog->println(entry.first.c_str());

    // Discover characteristics within the service
    std::map<std::string, BLERemoteCharacteristic *> *characteristics = pRemoteService->getCharacteristics();
    for (auto const &charEntry : *characteristics)
    {
      BLERemoteCharacteristic *pCharacteristic = charEntry.second;

      // Check if the characteristic can notify (Tx)
      if (pCharacteristic->canNotify() && liveData->pRemoteCharacteristic == nullptr)
      {
        liveData->pRemoteCharacteristic = pCharacteristic;
        syslog->print("Detected Tx characteristic UUID: ");
        syslog->println(charEntry.first.c_str());
      }

      // Check if the characteristic can write (Rx)
      if (pCharacteristic->canWrite() && liveData->pRemoteCharacteristicWrite == nullptr)
      {
        liveData->pRemoteCharacteristicWrite = pCharacteristic;
        syslog->print("Detected Rx characteristic UUID: ");
        syslog->println(charEntry.first.c_str());
      }

      // Stop searching if both Tx and Rx characteristics are found
      if (liveData->pRemoteCharacteristic && liveData->pRemoteCharacteristicWrite)
      {
        break;
      }
    }

    // Exit if we found valid Tx and Rx characteristics
    if (liveData->pRemoteCharacteristic && liveData->pRemoteCharacteristicWrite)
    {
      break;
    }
  }

  // Check if Tx and Rx characteristics were found
  if (liveData->pRemoteCharacteristic == nullptr || liveData->pRemoteCharacteristicWrite == nullptr)
  {
    syslog->println("Failed to detect Tx/Rx characteristics.");
    liveData->pClient->disconnect();
    return false;
  }

  syslog->println("Successfully detected service and characteristics.");

  // Enable notifications for the Tx characteristic
  if (liveData->pRemoteCharacteristic->canNotify())
  {
    const uint8_t indicationOn[] = {0x2, 0x0};
    BLERemoteDescriptor *notifyDescriptor = liveData->pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902));
    if (notifyDescriptor != nullptr)
    {
      notifyDescriptor->writeValue((uint8_t *)indicationOn, 2, true);
    }
    else
    {
      syslog->println("Notify descriptor 0x2902 not found. Registering callback only.");
    }
    liveData->pRemoteCharacteristic->registerForNotify(notifyCallback, false);
    delay(200);
  }

  syslog->println("BLE device is ready for communication.");
  return true;
}

/**
   Main loop
*/
void CommObd2Ble4::mainLoop()
{
  if (liveData->params.stopCommandQueue || suspendedDevice)
  {
    return;
  }

  // Connect BLE device
  if (liveData->obd2ready == true && hasConfiguredBleMac(liveData->settings.obdMacAddress))
  {
    if (liveData->menuVisible)
    {
      // Avoid blocking connect attempts while user is interacting with menu.
      connectStatus = "BLE connect paused (menu)";
    }
    else
    {
      const uint32_t nowMs = millis();
      const bool retryWindowOpen =
          (nextConnectRetryMs == 0) || (static_cast<int32_t>(nowMs - nextConnectRetryMs) >= 0);
      if (retryWindowOpen)
      {
        BLEAddress serverAddress(liveData->settings.obdMacAddress);
        if (connectToServer(serverAddress))
        {

          liveData->commConnected = true;
          liveData->obd2ready = false;
          connectFailCount = 0;
          nextConnectRetryMs = 0;

          syslog->println("We are now connected to the BLE device.");
          connectStatus = "Connected";

          // Print message
          board->displayMessage(" > Processing init AT cmds", "");

          // Serve first command (ATZ)
          doNextQueueCommand();
        }
        else
        {
          board->displayMessage("> Can not connect BLE", "");
          syslog->println("We have failed to connect to the server; scheduling retry.");
          if (connectFailCount < 0xFF)
            connectFailCount++;
          const uint32_t retryDelayMs = calcConnectRetryDelayMs(connectFailCount);
          nextConnectRetryMs = nowMs + retryDelayMs;
          connectStatus = String("Retry in ") + String(retryDelayMs / 1000) + "s";
        }
      }
    }
  }

  // Parent declaration
  CommInterface::mainLoop();

  if (board->scanDevices)
  {
    board->scanDevices = false;
    startBleScan();
  }
}

/**
 * Send command
 */
void CommObd2Ble4::executeCommand(String cmd)
{

  String tmpStr = cmd + "\r";
  if (liveData->commConnected)
  {
    liveData->pRemoteCharacteristicWrite->writeValue(tmpStr.c_str(), tmpStr.length());
  }
}

/**
 * Suspends the CAN device by setting it to sleep mode.
 * Stops communication and minimizes power consumption.
 */
void CommObd2Ble4::suspendDevice()
{
  suspendedDevice = true;
  liveData->commConnected = false;
  liveData->obd2ready = false;
  connectFailCount = 0;
  nextConnectRetryMs = 0;
  if (liveData->pClient != nullptr && liveData->pClient->isConnected())
  {
    liveData->pClient->disconnect();
  }
  if (liveData->pBLEScan != nullptr)
  {
    liveData->pBLEScan->stop();
  }
  connectStatus = "Suspended";
}

/**
 * Resumes the CAN device by reinitializing it to normal mode.
 */
void CommObd2Ble4::resumeDevice()
{
  suspendedDevice = false;
  liveData->obd2ready = true;
  connectFailCount = 0;
  nextConnectRetryMs = 0;
  connectStatus = "Resumed";
}
