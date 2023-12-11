#include "CommObd2Bt3.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define BT_DISCOVER_TIME 10000

/**
   Connect BT3 adapter
*/
void CommObd2Bt3::connectDevice()
{
  syslog->println("OBD2 BT3 connectDevice");

  ELM_PORT.begin();
  ELM_PORT.setPin("1234");
}

/**
   Disconnect device
*/
void CommObd2Bt3::disconnectDevice()
{

  syslog->println("COMM disconnectDevice");
  ELM_PORT.disconnect();
}

/**
   Scan device list, from menu
*/
void CommObd2Bt3::scanDevices()
{

  syslog->println("COMM scanDevices");
  startBt3Scan();
}

///////////////////////////////////

/**
   Start ble scan
*/
void CommObd2Bt3::startBt3Scan()
{
  String tmpStr;
  liveData->foundMyBleDevice = NULL;
  liveData->scanningDeviceIndex = 0;
  board->displayMessage(" > Scanning BT3 devices", "Wait 10 seconds");

  BTScanResults *btDeviceList = SerialBT.discover(BT_DISCOVER_TIME);
  if (btDeviceList)
  {
    if (btDeviceList->getCount() > 0)
    {
      BTAddress addr;
      int channel = 0;
      syslog->println("Found devices:");
      for (int i = 0; i < btDeviceList->getCount(); i++)
      {
        BTAdvertisedDevice *device = btDeviceList->getDevice(i);
        syslog->printf(" ----- %s  %s %d\n", device->getAddress().toString().c_str(), device->getName().c_str(), device->getRSSI());
        std::map<int, std::string> channels = SerialBT.getChannels(device->getAddress());
        syslog->printf("scanned for services, found %d\n", channels.size());
        for (auto const &entry : channels)
        {
          syslog->printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
        }
        if (channels.size() > 0)
        {
          addr = device->getAddress();
          channel = channels.begin()->first;
        }
        //
        if (liveData->scanningDeviceIndex < 10)
        {
          for (uint16_t i = 0; i < liveData->menuItemsCount; ++i)
          {
            if (liveData->menuItems[i].id == 10001 + liveData->scanningDeviceIndex)
            {
              tmpStr = device->getName().c_str();
              tmpStr.toCharArray(liveData->menuItems[i].title, 48);
              tmpStr = device->getAddress().toString().c_str();
              tmpStr.toCharArray(liveData->menuItems[i].obdMacAddress, 18);
            }
          }
          liveData->scanningDeviceIndex++;
        }
      }
      if (addr)
      {
        syslog->printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
        //        SerialBT.connect(addr, channel, sec_mask, role);
      }
    }
    else
    {
      board->displayMessage(" > Error on BT Scan", "No result!");
      syslog->println("Error on BT Scan, no result!");
      return;
    }
  }
  // Scan devices from menu, show list of devices
  if (liveData->menuCurrent == 9999)
  {
    syslog->println("Display menu with devices");
    liveData->menuVisible = true;
    // liveData->menuCurrent = 9999;
    liveData->menuItemSelected = 0;
    board->showMenu();
  }
}

/**
   Main loop
*/
void CommObd2Bt3::mainLoop()
{

  // Connect BLE device
  if (liveData->obd2ready == true)
  {
    syslog->print("Attempting to connect to ELM327... ");
    syslog->println(liveData->settings.obdMacAddress);

    BTAddress address(liveData->settings.obdMacAddress);
    if (ELM_PORT.connect("OBDII")) // *liveData->pServerAddress
    {
      syslog->print("Is bt connected ");
      if (!ELM_PORT.connected(3000))
      {
        ELM_PORT.println("Failed to connect. Make sure remote device is available and in range, then restart app.");
        return;
      }
      liveData->commConnected = false;
      liveData->obd2ready = true;
      syslog->println("We are now connected to the BT3 device.");

      // Print message
      board->displayMessage(" > Processing init AT cmds", "");

      // Serve first command (ATZ)
      doNextQueueCommand();
    }
    else
    {
      syslog->println("We have failed to connect to the server; there is nothing more we will do.");
      connectAttempts--;
      liveData->obd2ready = false;
    }
  }

  // Parent declaration
  CommInterface::mainLoop();

  /*if (board->scanDevices) {
    board->scanDevices = false;
    startBleScan();
  }*/

  // Prevent errors without connected module
  if (liveData->params.stopCommandQueue || !liveData->commConnected)
  {
    return;
  }

  if (ELM_PORT.available())
  {
    char c = ELM_PORT.read();
    if (c != '\r')
    {
      liveData->responseRowMerged += c;
    }
    else
    {
      syslog->infoNolf(DEBUG_COMM, "merged:");
      syslog->info(DEBUG_COMM, liveData->responseRowMerged);
      parseRowMerged();
      liveData->responseRowMerged = "";
      liveData->canSendNextAtCommand = true;
    }
  }
}

/**
 * Send command
 */
void CommObd2Bt3::executeCommand(String cmd)
{

  String tmpStr = cmd + "\r";
  if (liveData->commConnected)
  {
    ELM_PORT.print(tmpStr);
  }
}
