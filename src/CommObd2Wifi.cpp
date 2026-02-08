/**
 * Connects to the WiFi OBD2 adapter. Sets status flags and prints log messages.
 * Scans for available WiFi networks and prints results.
 * Main loop handles connecting to the OBD2 WiFi adapter, reading data,
 * parsing responses, and sending next commands. Sends commands to the OBD2 adapter.
 */
#include "CommObd2Wifi.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include "WiFi.h"
#include "WiFiClient.h"

WiFiClient client;

namespace
{
  constexpr uint32_t kWifiRetryIntervalMs = 8000;
  constexpr uint32_t kTcpRetryIntervalMs = 1500;
  constexpr uint32_t kObdCommandTimeoutMs = 2500;

  bool wifiSsidConfigured(const LiveData *liveData)
  {
    return liveData->settings.wifiSsid[0] != '\0' &&
           strcmp(liveData->settings.wifiSsid, "empty") != 0 &&
           strcmp(liveData->settings.wifiSsid, "not_set") != 0;
  }
} // namespace

/**
   Connect Wifi adapter
*/
void CommObd2Wifi::connectDevice()
{
  syslog->println("OBD2 Wifi connectDevice");

  liveData->obd2ready = true;
  liveData->commConnected = false;
  lastWifiRetryMs = 0;
  lastTcpRetryMs = 0;
  lastDataSent = 0;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  liveData->canSendNextAtCommand = false;

  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  if (wifiSsidConfigured(liveData))
  {
    WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);
    connectStatus = "Connecting WiFi...";
  }
  else
  {
    connectStatus = "Set WiFi SSID";
  }
}

/**
   Disconnect device
*/
void CommObd2Wifi::disconnectDevice()
{
  syslog->println("COMM disconnectDevice");
  client.stop();
  liveData->commConnected = false;
  liveData->obd2ready = true;
  lastDataSent = 0;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
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
  if (liveData->params.stopCommandQueue || suspendedDevice)
  {
    return;
  }

  if (liveData->commConnected && !client.connected())
  {
    client.stop();
    liveData->commConnected = false;
    liveData->obd2ready = true;
    connectStatus = "OBD2 WiFi disconnected";
  }

  // Connect WiFi OBD2 adapter.
  if (liveData->obd2ready == true)
  {
    const uint32_t nowMs = millis();
    if (WiFi.status() != WL_CONNECTED)
    {
      if (!wifiSsidConfigured(liveData))
      {
        connectStatus = "Set WiFi SSID";
        return;
      }

      if (lastWifiRetryMs == 0 || nowMs - lastWifiRetryMs >= kWifiRetryIntervalMs)
      {
        lastWifiRetryMs = nowMs;
        connectStatus = "Connecting WiFi...";
        WiFi.enableSTA(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(liveData->settings.wifiSsid, liveData->settings.wifiPassword);
      }
      return;
    }

    if (lastTcpRetryMs != 0 && nowMs - lastTcpRetryMs < kTcpRetryIntervalMs)
    {
      return;
    }
    lastTcpRetryMs = nowMs;

    // Connect stream
    syslog->println("Connect obd2 wifi client.");
    if (!client.connect(liveData->settings.obd2WifiIp, liveData->settings.obd2WifiPort))
    {
      connectStatus = "OBD2 TCP failed. Check IP/port";
      syslog->println("OBD2 TCP failed. Trying fallback port.");
      if (!client.connect(liveData->settings.obd2WifiIp, (liveData->settings.obd2WifiPort == 23 ? 35000 : 23)))
      {
        return;
      }
    }

    connectStatus = "Connected...";
    syslog->println("We are now connected to the Wifi device.");
    client.setNoDelay(true);
    liveData->commConnected = true;
    liveData->obd2ready = false;
    lastTcpRetryMs = 0;
    lastDataSent = 0;
    liveData->responseRow = "";
    liveData->responseRowMerged = "";
    liveData->canSendNextAtCommand = false;

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

  auto finalizeResponse = [this]() {
    String row = liveData->responseRow;
    row.trim();
    if (row.length() > 0 && row != ">")
    {
      liveData->responseRow = row;
      parseResponse();
    }
    liveData->responseRow = "";
  };

  auto handlePrompt = [this, &finalizeResponse]() {
    finalizeResponse();
    if (liveData->responseRowMerged != "")
    {
      syslog->infoNolf(DEBUG_COMM, "merged:");
      syslog->info(DEBUG_COMM, liveData->responseRowMerged);
      parseRowMerged();
    }
    liveData->responseRowMerged = "";
    liveData->lastCommandLatencyMs = (lastDataSent == 0) ? 0 : (millis() - lastDataSent);
    lastDataSent = 0;
    liveData->canSendNextAtCommand = true;
  };

  while (client.available())
  {
    char ch = client.read();

    if (ch == '>')
    {
      handlePrompt();
      continue;
    }

    if (ch == '\r' || ch == '\n' || ch == '\0')
    {
      if (liveData->responseRow == ">")
      {
        liveData->responseRow = "";
        handlePrompt();
        continue;
      }
      finalizeResponse();
    }
    else
    {
      liveData->responseRow += ch;
    }
  }

  if (!liveData->canSendNextAtCommand && lastDataSent != 0)
  {
    uint32_t timeoutMs = liveData->rxTimeoutMs;
    if (timeoutMs < kObdCommandTimeoutMs)
    {
      timeoutMs = kObdCommandTimeoutMs;
    }

    if ((unsigned long)(millis() - lastDataSent) > timeoutMs)
    {
      syslog->info(DEBUG_COMM, "WiFi OBD timeout. Continue with next command.");
      String row = liveData->responseRow;
      row.trim();
      if (row.length() > 0 && row != ">")
      {
        liveData->responseRow = row;
        parseResponse();
      }
      if (liveData->responseRowMerged != "")
      {
        parseRowMerged();
      }
      liveData->responseRow = "";
      liveData->responseRowMerged = "";
      liveData->lastCommandLatencyMs = millis() - lastDataSent;
      lastDataSent = 0;
      liveData->canSendNextAtCommand = true;
      connectStatus = "OBD2 timeout";
    }
  }
}

/**
 * Send command
 */
void CommObd2Wifi::executeCommand(String cmd)
{
  String tmpStr = cmd + '\r';
  if (liveData->commConnected)
  {
    lastDataSent = millis();
    liveData->lastCommandLatencyMs = 0;
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
  liveData->commConnected = false;
  liveData->obd2ready = false;
  client.stop();
  lastDataSent = 0;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  connectStatus = "Suspended";
}

/**
 * Resumes the CAN device by reinitializing it to normal mode.
 */
void CommObd2Wifi::resumeDevice()
{
  suspendedDevice = false;
  lastWifiRetryMs = 0;
  lastTcpRetryMs = 0;
  lastDataSent = 0;
  liveData->obd2ready = true;
  liveData->commConnected = false;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  liveData->canSendNextAtCommand = false;
  connectStatus = "Resumed";
}
