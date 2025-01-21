/**
 * Connects to the WiFi OBD2 adapter. Sets status flags and prints log messages.
 * Scans for available WiFi networks and prints results.
 * Main loop handles connecting to the OBD2 WiFi adapter, reading data,
 * parsing responses, and sending next commands. Sends commands to the OBD2 adapter.
 */
#include "CommObd2Wifi.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include "Wifi.h"
#include "WifiClient.h"

WiFiClient client;

/**
   Connect Wifi adapter
*/
void CommObd2Wifi::connectDevice()
{
  syslog->println("OBD2 Wifi connectDevice");

  liveData->obd2ready = true;
  liveData->commConnected = false;
}

/**
   Disconnect device
*/
void CommObd2Wifi::disconnectDevice()
{
  syslog->println("COMM disconnectDevice");
  client.stop();
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
   Start wifi scan
*/
void CommObd2Wifi::startWifiScan()
{
  // todo
  syslog->println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  syslog->println("scan done");
  if (n == 0)
  {
    syslog->println("no networks found");
  }
  else
  {
    syslog->print(n);
    syslog->println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      syslog->print(i + 1);
      syslog->print(": ");
      syslog->print(WiFi.SSID(i));
      syslog->print(" (");
      syslog->print(WiFi.RSSI(i));
      syslog->print(")");
      syslog->println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  syslog->println("");
}

/**
 * Main loop
 **/
void CommObd2Wifi::mainLoop()
{
  // Connect BLE device
  if (liveData->obd2ready == true)
  {
    // Check wifi ip address
    connectStatus = "WIFI device not connected";
    if (WiFi.localIP().toString() == "")
    {
      return;
    }

    // Connect stream
    Serial.println("Connect obd2 wifi client.");
    if (!client.connect(liveData->settings.obd2WifiIp, liveData->settings.obd2WifiPort))
    {
      connectStatus = "Connection failed. Ip/port?";
      Serial.println("Connection failed. Ip/port?");
      if (!client.connect(liveData->settings.obd2WifiIp, (liveData->settings.obd2WifiPort == 23 ? 35000 : 23)))
      {
        return;
      }
    }

    connectStatus = "Connected...";
    syslog->println("We are now connected to the Wifi device.");
    liveData->commConnected = true;
    liveData->obd2ready = false;

    // Print message
    board->displayMessage(" > Processing init AT cmds", "");

    // Serve first command (ATZ)
    doNextQueueCommand();
  }

  // Parent declaration
  CommInterface::mainLoop();

  // Prevent errors without connected module
  if (liveData->params.stopCommandQueue || !liveData->commConnected)
  {
    return;
  }

  while (client.available())
  {
    char ch = client.read();
    if (ch == '\r' || ch == '\n' || ch == '\0')
    {
      if (liveData->responseRow != "")
        parseResponse();
      liveData->responseRow = "";
    }
    else
    {
      liveData->responseRow += ch;
      if (liveData->responseRow == ">")
      {
        if (liveData->responseRowMerged != "")
        {
          syslog->infoNolf(DEBUG_COMM, "merged:");
          syslog->info(DEBUG_COMM, liveData->responseRowMerged);
          parseRowMerged();
        }
        liveData->responseRowMerged = "";
        liveData->canSendNextAtCommand = true;
      }
    }
  }
}

/**
 * Send command
 */
void CommObd2Wifi::executeCommand(String cmd)
{
  cmd.replace(" ", "");
  String tmpStr = cmd + '\r';
  if (liveData->commConnected)
  {
    client.print(tmpStr);
  }
}

/**
 * Suspends the CAN device by setting it to sleep mode.
 * Stops communication and minimizes power consumption.
 */
void CommObd2Wifi::suspendDevice()
{
  suspendedDevice = true;
  // TODO
}

/**
 * Resumes the CAN device by reinitializing it to normal mode.
 */
void CommObd2Wifi::resumeDevice()
{
  suspendedDevice = false;
  // TODO
}