#include "CommObd2Bt3.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include <stdio.h>

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_CLASSIC_ENABLED) && defined(CONFIG_BT_SPP_ENABLED)
#include <BluetoothSerial.h>
#define EVDASH_BT3_AVAILABLE 1
#else
#define EVDASH_BT3_AVAILABLE 0
#endif

namespace
{
  constexpr uint32_t kBtRetryIntervalMs = 7000;
  constexpr uint32_t kObdCommandTimeoutMs = 2500;
  constexpr const char *kBtLocalName = "evDash";
  constexpr const char *kBtDefaultPin = "1234";

#if EVDASH_BT3_AVAILABLE
  BluetoothSerial serialBt3;
#endif

  bool isConfiguredMac(const char *value)
  {
    return value != nullptr &&
           value[0] != '\0' &&
           strcmp(value, "00:00:00:00:00:00") != 0 &&
           strcmp(value, "not_set") != 0 &&
           strcmp(value, "empty") != 0;
  }

  bool isConfiguredName(const char *value)
  {
    return value != nullptr &&
           value[0] != '\0' &&
           strcmp(value, "OBD2") != 0 &&
           strcmp(value, "not_set") != 0 &&
           strcmp(value, "empty") != 0;
  }

  bool parseMacAddress(const char *value, uint8_t outAddr[6])
  {
    if (value == nullptr || outAddr == nullptr)
    {
      return false;
    }

    unsigned int parsed[6] = {0, 0, 0, 0, 0, 0};
    if (sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x",
               &parsed[0], &parsed[1], &parsed[2], &parsed[3], &parsed[4], &parsed[5]) != 6)
    {
      return false;
    }

    for (uint8_t i = 0; i < 6; i++)
    {
      if (parsed[i] > 0xFF)
      {
        return false;
      }
      outAddr[i] = static_cast<uint8_t>(parsed[i]);
    }

    return true;
  }
} // namespace

void CommObd2Bt3::connectDevice()
{
  syslog->println("OBD2 BT3 connectDevice");

  liveData->obd2ready = true;
  liveData->commConnected = false;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  liveData->canSendNextAtCommand = false;
  lastDataSent = 0;
  lastConnectRetryMs = 0;
  btStackReady = false;
  connectAttemptPending = true;

#if EVDASH_BT3_AVAILABLE
  serialBt3.end();
  if (!serialBt3.begin(kBtLocalName, true))
  {
    connectStatus = "OBD2 BT3 init failed";
    return;
  }

  serialBt3.setPin(kBtDefaultPin);
  btStackReady = true;
  connectStatus = "OBD2 BT3 ready";
#else
  connectStatus = "BT3 unsupported on this board";
#endif
}

void CommObd2Bt3::disconnectDevice()
{
  syslog->println("COMM disconnectDevice");

#if EVDASH_BT3_AVAILABLE
  serialBt3.disconnect();
  serialBt3.end();
#endif

  btStackReady = false;
  connectAttemptPending = false;
  liveData->commConnected = false;
  liveData->obd2ready = true;
  liveData->canSendNextAtCommand = false;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  lastDataSent = 0;
}

void CommObd2Bt3::scanDevices()
{
  syslog->println("COMM BT3 scanDevices");
  board->displayMessage("BT3 scan not ready", "Set BT name or MAC");
}

void CommObd2Bt3::mainLoop()
{
  if (liveData->params.stopCommandQueue || suspendedDevice)
  {
    return;
  }

#if !EVDASH_BT3_AVAILABLE
  return;
#else
  const uint32_t nowMs = millis();

  if (!btStackReady)
  {
    if (lastConnectRetryMs == 0 || static_cast<uint32_t>(nowMs - lastConnectRetryMs) >= kBtRetryIntervalMs)
    {
      lastConnectRetryMs = nowMs;
      if (serialBt3.begin(kBtLocalName, true))
      {
        serialBt3.setPin(kBtDefaultPin);
        btStackReady = true;
        connectStatus = "OBD2 BT3 ready";
      }
      else
      {
        connectStatus = "OBD2 BT3 init failed";
      }
    }
    return;
  }

  if (liveData->commConnected && !serialBt3.connected())
  {
    liveData->commConnected = false;
    liveData->obd2ready = true;
    connectAttemptPending = true;
    liveData->canSendNextAtCommand = false;
    liveData->responseRow = "";
    liveData->responseRowMerged = "";
    lastDataSent = 0;
    connectStatus = "OBD2 BT3 disconnected";
  }

  if (liveData->obd2ready == true)
  {
    if (!connectAttemptPending)
    {
      return;
    }

    if (liveData->menuVisible)
    {
      // Avoid long blocking BT connect attempts while user is navigating menu.
      return;
    }

    if (lastConnectRetryMs != 0 && static_cast<uint32_t>(nowMs - lastConnectRetryMs) < kBtRetryIntervalMs)
    {
      return;
    }
    lastConnectRetryMs = nowMs;
    connectAttemptPending = false;

    bool connected = false;
    if (isConfiguredMac(liveData->settings.obdMacAddress))
    {
      uint8_t targetMac[6] = {0, 0, 0, 0, 0, 0};
      if (parseMacAddress(liveData->settings.obdMacAddress, targetMac))
      {
        connected = serialBt3.connect(targetMac);
      }
      else
      {
        syslog->println("BT3: invalid MAC format in settings.");
      }
    }

    if (!connected && isConfiguredName(liveData->settings.obd2Name))
    {
      connected = serialBt3.connect(String(liveData->settings.obd2Name));
    }

    if (!connected)
    {
      if (!isConfiguredMac(liveData->settings.obdMacAddress) &&
          !isConfiguredName(liveData->settings.obd2Name))
      {
        connectStatus = "Set BT name or MAC";
      }
      else
      {
        connectStatus = "OBD2 BT3 connect failed";
      }
      return;
    }

    connectStatus = "Connected...";
    syslog->println("We are now connected to the BT3 device.");
    liveData->commConnected = true;
    liveData->obd2ready = false;
    liveData->canSendNextAtCommand = false;
    liveData->responseRow = "";
    liveData->responseRowMerged = "";
    lastDataSent = 0;
    lastConnectRetryMs = 0;

    board->displayMessage(" > Processing init AT cmds", "");
    doNextQueueCommand();
  }

  // Parent declaration
  CommInterface::mainLoop();

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

  while (serialBt3.available())
  {
    char ch = serialBt3.read();

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
      syslog->info(DEBUG_COMM, "BT3 OBD timeout. Continue with next command.");
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
#endif
}

void CommObd2Bt3::executeCommand(String cmd)
{
#if EVDASH_BT3_AVAILABLE
  String tmpStr = cmd + '\r';
  if (liveData->commConnected)
  {
    lastDataSent = millis();
    liveData->lastCommandLatencyMs = 0;
    serialBt3.print(tmpStr);
  }
#else
  (void)cmd;
#endif
}

void CommObd2Bt3::suspendDevice()
{
  suspendedDevice = true;
  connectAttemptPending = false;
  liveData->commConnected = false;
  liveData->obd2ready = false;
  liveData->canSendNextAtCommand = false;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  lastDataSent = 0;

#if EVDASH_BT3_AVAILABLE
  serialBt3.disconnect();
#endif

  connectStatus = "Suspended";
}

void CommObd2Bt3::resumeDevice()
{
  suspendedDevice = false;
  connectAttemptPending = true;
  liveData->obd2ready = true;
  liveData->commConnected = false;
  liveData->canSendNextAtCommand = false;
  liveData->responseRow = "";
  liveData->responseRowMerged = "";
  lastDataSent = 0;
  lastConnectRetryMs = 0;

#if EVDASH_BT3_AVAILABLE
  if (!btStackReady)
  {
    btStackReady = serialBt3.begin(kBtLocalName, true);
    if (btStackReady)
    {
      serialBt3.setPin(kBtDefaultPin);
    }
    else
    {
      connectStatus = "OBD2 BT3 init failed";
      return;
    }
  }
#endif

  connectStatus = "Resumed";
}
