#include "CommInterface.h"
#include "BoardInterface.h"
//#include "CarInterface.h"
#include "LiveData.h"

/**
  init
*/
void CommInterface::initComm(LiveData *pLiveData, BoardInterface *pBoard)
{

  liveData = pLiveData;
  board = pBoard;
  response = "";
}

/**
  Main loop
*/
void CommInterface::mainLoop()
{

  // Send command from TTY to OBD2
  if (syslog->available())
  {
    ch = syslog->read();
    if (ch == '\r' || ch == '\n')
    {
      board->customConsoleCommand(response);
      response = response + ch;
      syslog->info(DEBUG_COMM, response);
      executeCommand(response);
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

  // Can send next command from queue to OBD
  if (liveData->canSendNextAtCommand)
  {
    liveData->canSendNextAtCommand = false;
    doNextQueueCommand();
  }
}

/**
  Do next AT command from queue
*/
bool CommInterface::doNextQueueCommand()
{

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

    // Queue optimizer
    commandAllowed = board->carCommandAllowed();
    liveData->commandQueueIndex++;
    if (liveData->commandQueueIndex >= liveData->commandQueueCount)
    {
      liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
      liveData->params.queueLoopCounter++;
      board->redrawScreen();

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
  executeCommand(liveData->commandRequest);

  return true;
}

/**
  Parse result from OBD, merge frames to sigle response
*/
bool CommInterface::parseResponse()
{

  // 1 frame data
  syslog->info(DEBUG_COMM, liveData->responseRow);

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
   Send final response to board
*/
void CommInterface::parseRowMerged()
{

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
   CanComparer - record whole queue loop
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
   CanComparer - compare captured results (0..3) and print byte differences (0 = 2, 1 = 3, 0 <> 1)
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
