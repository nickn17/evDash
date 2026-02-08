/**
 * CommInterface class handles communication with the vehicle's OBD port.
 * initComm() initializes the communication interface.
 * checkConnectAttempts() checks if any connection attempts were made.
 * getConnectAttempts() gets the number of connection attempts.
 * getConnectStatus() gets the connection status string.
 * mainLoop() runs the main communication loop, sending commands and parsing responses.
 * doNextQueueCommand() gets the next command from the queue and sends it.
 * parseResponse() parses response frames into a single response string.
 * parseRowMerged() handles the merged response string.
 * recordLoop() records a full command queue loop for later comparison.
 * compareCanRecords() compares recorded command loop results.
 * sendPID() sends a PID command.
 * receivePID() receives and handles a PID response.
 */
#include "CommInterface.h"
#include "BoardInterface.h"
// #include "CarInterface.h"
#include "LiveData.h"

/**
 * Initializes the communication interface by setting the live data and board references.
 */
void CommInterface::initComm(LiveData *pLiveData, BoardInterface *pBoard)
{
  liveData = pLiveData;
  board = pBoard;
  response = "";
}

/**
 * checkConnectAttempts
 */
uint8_t CommInterface::checkConnectAttempts()
{
  if (connectAttempts == 0)
  {
    connectStatus = "CAN failed. 0 attempts";
    suspendDevice();
  }
  return connectAttempts > 0;
}

/**
 * getConnectAttempts
 */
int8_t CommInterface::getConnectAttempts()
{
  return connectAttempts;
}

/**
 * getConnectStatus
 */
String CommInterface::getConnectStatus()
{
  return connectStatus;
}

/**
 * mainLoop() is the main loop that runs continuously to process serial input commands,
 * update live data, send next CAN command from queue if ready, and handle starting odometer
 * and energy counters. It reads any serial input, executes commands when newline received,
 * checks CAN send cooldown timer, sends next queued CAN command when ready, initializes
 * start values for odometer and energy counters if needed, and resets chargingOn flag if
 * stale.
 */
void CommInterface::mainLoop()
{
  const bool queueSuspended = liveData->params.stopCommandQueue || suspendedDevice;

  // Send command from TTY to OBD2
  if (syslog->available())
  {
    ch = syslog->read();
    if (ch == '\r' || ch == '\n')
    {
      board->customConsoleCommand(response);

      // raise dump noo needed, work is done by customConsoleCommand method
      // Lubos: Not exactly. This is for AT/CAN commands via serial console.
      // CustomConsoleCommand only process evDash command (reboot, savesetting, etd).
      response = response + ch;
      syslog->info(DEBUG_COMM, response);
      if (!queueSuspended)
      {
        executeCommand(response);
      }
      response = "";
    }
    else
    {
      response = response + ch;
    }
  }

  // Set start counters (odoKm, total charged, total discharged)
  if (liveData->params.odoKmStart == -1 && liveData->params.odoKm != -1)
    liveData->params.odoKmStart = liveData->params.odoKm;
  if (liveData->params.cumulativeEnergyChargedKWhStart == -1 && liveData->params.cumulativeEnergyChargedKWh != -1)
    liveData->params.cumulativeEnergyChargedKWhStart = liveData->params.cumulativeEnergyChargedKWh;
  if (liveData->params.cumulativeEnergyDischargedKWhStart == -1 && liveData->params.cumulativeEnergyDischargedKWh != -1)
    liveData->params.cumulativeEnergyDischargedKWhStart = liveData->params.cumulativeEnergyDischargedKWh;

  // Drop ChargingOn when status was not updated for more than 10 seconds
  if (liveData->params.currentTime - liveData->params.lastChargingOnTime > 10 && liveData->params.chargingOn)
    liveData->params.chargingOn = false;

  // No CAN response (timeout)
  if (liveData->settings.commType == 1 &&
      liveData->params.currentTime - liveData->params.lastCanbusResponseTime > 5 &&
      checkConnectAttempts())
  {
    connectStatus = "No CAN response";
  }

  // Can send next command from queue to OBD
  if (liveData->canSendNextAtCommand)
  {
    if (queueSuspended)
    {
      connectStatus = "CAN queue paused";
      return;
    }
    if (liveData->commandQueueIndex == liveData->commandQueueCount && liveData->params.stopCommandQueue)
    {
      connectStatus = "CAN queue paused";
      syslog->println("CAN/OBD2 command queue stopped due to sleep mode & voltage...");
    }
    else
    {
      liveData->canSendNextAtCommand = false;
      doNextQueueCommand();
    }
  }
}

/**
 * Executes the next AT command from the command queue.
 * Checks if command is allowed based on car status.
 * Increments command queue index and loops back to start if at end.
 * Handles contribute data collection status.
 * Logs and executes allowed command.
 */
bool CommInterface::doNextQueueCommand()
{
  if (liveData->params.stopCommandQueue || suspendedDevice)
  {
    connectStatus = "CAN queue paused";
    liveData->canSendNextAtCommand = true;
    return false;
  }

  // Send AT command to obd
  bool commandAllowed = false;
  do
  {
    liveData->commandRequest = liveData->commandQueue[liveData->commandQueueIndex].request;
    liveData->commandStartChar = liveData->commandQueue[liveData->commandQueueIndex].startChar; // TODO: add to struct?
    if (liveData->commandRequest.startsWith("ATSH"))
    {
      liveData->currentAtshRequest = liveData->commandRequest;
    }

    // Contribute data flags
    if (liveData->commandQueueIndex == liveData->commandQueueLoopFrom)
    {
      if (liveData->params.contributeStatus == CONTRIBUTE_COLLECTING)
      {
        syslog->println("contributeStatus ... ready to send");
        liveData->params.contributeStatus = CONTRIBUTE_READY_TO_SEND;
      }
      if (liveData->params.contributeStatus == CONTRIBUTE_WAITING)
      {
        syslog->println("contributeStatus ... collecting data");

        liveData->params.contributeStatus = CONTRIBUTE_COLLECTING;
        liveData->clearContributeRawFrames();
        if (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V1)
        {
          liveData->contributeDataJson = "{"; // begin json
        }
      }
    }

    if (liveData->params.contributeStatus == CONTRIBUTE_COLLECTING &&
        liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V1)
    {
      static uint32_t lastUiYieldMs = 0;
      uint32_t nowMs = millis();
      if (nowMs - lastUiYieldMs >= 200)
      {
        lastUiYieldMs = nowMs;
        liveData->redrawScreenRequested = true;
        if (board)
        {
          board->redrawScreen();
        }
      }
    }

    // Queue optimizer
    commandAllowed = board->carCommandAllowed();
    liveData->commandQueueIndex++;

    if (liveData->commandQueueIndex >= liveData->commandQueueCount)
    {
      liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
      liveData->params.queueLoopCounter++;
      liveData->redrawScreenRequested = true;
      // board->redrawScreen();

      // log every queue loop (temp) TODO rewrite to secs interval
      liveData->params.sdcardCanNotify = true;
    }

    // Log skipped command to console
    if (!commandAllowed)
    {
      syslog->infoNolf(DEBUG_COMM, ">>> Command skipped ");
      syslog->info(DEBUG_COMM, liveData->commandRequest);
    }

  } while (!commandAllowed);

  // Execute command
  syslog->infoNolf(DEBUG_COMM, ">>> ");
  syslog->info(DEBUG_COMM, liveData->commandRequest);
  liveData->responseRowMerged = "";
  liveData->vResponseRowMerged.clear();
  executeCommand(liveData->commandRequest);

  return true;
}

/**
 * Parses response frames from OBD into a single merged response string.
 * Handles merging multi-line responses into a single string, as well as
 * passing through single line responses.
 */
bool CommInterface::parseResponse()
{
  // 1 frame data
  if (liveData->responseRow.length() < 2 || (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) != ':'))
  {
    syslog->info(DEBUG_COMM, liveData->responseRow);
  }

  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':')
  {
    liveData->responseRowMerged += liveData->responseRow.substring(2);
  }
  else if (liveData->responseRow.length() >= 4)
  {
    liveData->responseRowMerged += liveData->responseRow;
  }

  return true;
}

/**
 * Parses the merged response string into individual responses.
 * Calls the board parseRowMerged() method to handle parsing.
 * Also handles anonymous data contribution and CAN comparer recording.
 */
void CommInterface::parseRowMerged()
{
  // Anonymous data
  if (liveData->params.contributeStatus == CONTRIBUTE_COLLECTING)
  {
    String contributeKey = liveData->currentAtshRequest + "_" + liveData->commandRequest;
    if (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V1)
    {
      liveData->contributeDataJson += "\"" + contributeKey + "\": \"" + liveData->responseRowMerged + "\", ";
      liveData->contributeDataJson += "\"" + contributeKey + "_ms\": \"" + String(liveData->lastCommandLatencyMs) + "\", ";
    }
    else if (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V2)
    {
      liveData->addContributeRawFrame(contributeKey, liveData->responseRowMerged, liveData->lastCommandLatencyMs);
    }

    if (liveData->packetFilteredPending)
    {
      if (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V1)
      {
        liveData->contributeDataJson += "\"packet_filtered_command\": \"" + liveData->packetFilteredCommand + "\", ";
        liveData->contributeDataJson += "\"packet_filtered_id\": \"" + liveData->packetFilteredId + "\", ";
        if (liveData->packetFilteredData.length())
          liveData->contributeDataJson += "\"packet_filtered_data\": \"" + liveData->packetFilteredData + "\", ";
      }
      else if (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V2)
      {
        liveData->addContributeRawFrame("packet_filtered_command", liveData->packetFilteredCommand, 0);
        liveData->addContributeRawFrame("packet_filtered_id", liveData->packetFilteredId, 0);
        if (liveData->packetFilteredData.length())
        {
          liveData->addContributeRawFrame("packet_filtered_data", liveData->packetFilteredData, 0);
        }
      }
      liveData->packetFilteredPending = false;
      liveData->packetFilteredCommand = "";
      liveData->packetFilteredId = "";
      liveData->packetFilteredData = "";
    }
    if (liveData->settings.contributeJsonType == CONTRIBUTE_JSON_TYPE_V1)
    {
      syslog->println(liveData->contributeDataJson);
    }
  }

  // Record previous response
  if (canComparerRecordIndex != -1)
  {
    if (canComparerRecordQueueLoop < liveData->params.queueLoopCounter)
    {
      syslog->print("canRec");
      syslog->print(canComparerRecordIndex);
      syslog->print(": ");
      syslog->println(canComparerData[canComparerRecordIndex]);
      canComparerRecordIndex = -1; // Stop recording
    }
    else if (!liveData->commandRequest.startsWith("AT"))
    {
      if (canComparerRecordQueueLoop == liveData->params.queueLoopCounter)
      {
        String key = "/" + liveData->currentAtshRequest + "/" + liveData->commandRequest + "/";
        if (canComparerRecordIndex == 0)
        {
          canComparerData[canComparerRecordIndex] += key + liveData->responseRowMerged;
        }
        else
        {
          uint16_t startPos = canComparerData[0].indexOf(key);
          if (startPos != -1)
          {
            canComparerData[canComparerRecordIndex] = canComparerData[canComparerRecordIndex].substring(0, startPos) + key +
                                                      liveData->responseRowMerged + canComparerData[canComparerRecordIndex].substring(startPos + key.length() + liveData->responseRowMerged.length());
          }
        }
      }
    }
  }

  liveData->params.lastCanbusResponseTime = liveData->params.currentTime;

  // Parse response
  board->parseRowMerged();
}

/**
 * Records the current queue loop to the specified data index
 * in the canComparerData array. This allows comparing the current
 * loop data to previous loops.
 */
void CommInterface::recordLoop(int8_t dataIndex)
{

  canComparerRecordIndex = dataIndex;
  if (canComparerRecordIndex == 0)
  {
    canComparerData[canComparerRecordIndex] = "";
  }
  else
  {
    uint16_t maxLen = canComparerData[0].length();
    canComparerData[canComparerRecordIndex] = "";
    for (uint16_t i = 0; i < maxLen; i++)
      canComparerData[canComparerRecordIndex] += "#";
  }
  canComparerRecordQueueLoop = liveData->params.queueLoopCounter + 1;

  syslog->print("canComparer: Recording loop to register: ");
  syslog->println(canComparerRecordIndex);
}

/**
 * Compares previously recorded CAN bus data loops.
 * Prints differences between loops 0 and 1 compared to 2 and 3.
 */
void CommInterface::compareCanRecords()
{
  syslog->println(canComparerData[0]);
  uint16_t maxLen = canComparerData[0].length();
  for (uint16_t i = 0; i < maxLen; i++)
  {
    syslog->print((canComparerData[0].charAt(i) != canComparerData[1].charAt(i) &&
                   canComparerData[0].charAt(i) == canComparerData[2].charAt(i) &&
                   canComparerData[1].charAt(i) == canComparerData[3].charAt(i))
                      ? "#"
                      : " ");
  }
  syslog->println();
  syslog->println(canComparerData[1]);
}

/**
 *
 */
void CommInterface::sendPID(const uint32_t pid, const String &cmd)
{
}

/**
 *
 */
uint8_t CommInterface::receivePID()
{
  return 0;
}

/**
 * Is device suspended?
 */
bool CommInterface::isSuspended()
{
  return suspendedDevice;
}
