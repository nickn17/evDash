#include "CommObd2Wifi.h"
#include "BoardInterface.h"
#include "LiveData.h"

/**
   Connect Wifi adapter
*/
void CommObd2Wifi::connectDevice()
{
  syslog->println("OBD2 Wifi connectDevice");

}

/**
   Disconnect device
*/
void CommObd2Wifi::disconnectDevice()
{
  syslog->println("COMM disconnectDevice");
}

/**
   Scan device list, from menu
*/
void CommObd2Wifi::scanDevices()
{
  syslog->println("COMM scanDevices");
  startWifiScan();
}

///////////////////////////////////

/**
   Start ble scan
*/
void CommObd2Wifi::startWifiScan()
{
  String tmpStr;
  liveData->foundMyBleDevice = NULL;
  liveData->scanningDeviceIndex = 0;
  board->displayMessage(" > Scanning Wifi devices", "Wait 10 seconds");

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
 * Main loop
 **/
void CommObd2Wifi::mainLoop()
{
  // Connect BLE device
  if (liveData->obd2ready == true)
  {
    syslog->print("Attempting to connect to ELM327... ");
    syslog->println(liveData->settings.obdMacAddress);

 /*   BTAddress address(liveData->settings.obdMacAddress);
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
      syslog->println("We are now connected to the Wifi device.");

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
    }*/
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

  /*if (ELM_PORT.available())
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
  }*/
}

/**
 * Send command
 */
void CommObd2Wifi::executeCommand(String cmd)
{

  String tmpStr = cmd + "\r";
  if (liveData->commConnected)
  {
//    ELM_PORT.print(tmpStr);
  }
}
