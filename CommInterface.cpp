#include "CommInterface.h"
#include "BoardInterface.h"
//#include "CarInterface.h"
#include "LiveData.h"

/**

*/
void CommInterface::initComm(LiveData* pLiveData, BoardInterface* pBoard) {

  liveData = pLiveData;
  board = pBoard;
  response = "";
}

/**
   Main loop
*/
void CommInterface::mainLoop() {

  // Send command from TTY to OBD2
  if (syslog->available()) {
    ch = syslog->read();
    if (ch == '\r' || ch == '\n') {
      board->customConsoleCommand(response);
      response = response + ch;
      syslog->info(DEBUG_COMM, response);
      executeCommand(response);
      response = "";
    } else {
      response = response + ch;
    }
  }

  // Drop ChargingOn when status was not updated for more than 10 seconds
  if (liveData->params.currentTime - liveData->params.lastChargingOnTime > 10 && liveData->params.chargingOn) {
    liveData->params.chargingOn = false;
  }

  // Can send next command from queue to OBD
  if (liveData->canSendNextAtCommand) {
    liveData->canSendNextAtCommand = false;
    doNextQueueCommand();
  }
}

/**
  Do next AT command from queue
*/
bool CommInterface::doNextQueueCommand() {

  // Send AT command to obd
  bool commandAllowed = false;
  do {
    liveData->commandRequest = liveData->commandQueue[liveData->commandQueueIndex].request;
    liveData->commandStartChar = liveData->commandQueue[liveData->commandQueueIndex].startChar; // TODO: add to struct?
    if (liveData->commandRequest.startsWith("ATSH")) {
      liveData->currentAtshRequest = liveData->commandRequest;
    }

    // Queue optimizer
    commandAllowed = board->carCommandAllowed();
    liveData->commandQueueIndex++;
    if (liveData->commandQueueIndex >= liveData->commandQueueCount) {
      liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
      liveData->params.queueLoopCounter++;
      board->redrawScreen();

      // log every queue loop (temp) TODO rewrite to secs interval
      liveData->params.sdcardCanNotify = true;
    }

    // Log skipped command to console
    if (!commandAllowed) {
      syslog->infoNolf(DEBUG_COMM, ">>> Command skipped ");
      syslog->info(DEBUG_COMM, liveData->commandRequest);
    }

  } while (!commandAllowed);

  // Execute command
  syslog->infoNolf(DEBUG_COMM, ">>> ");
  syslog->info(DEBUG_COMM, liveData->commandRequest);
  liveData->responseRowMerged = "";
  executeCommand(liveData->commandRequest);
}

/**
  Parse result from OBD, merge frames to sigle response
*/
bool CommInterface::parseResponse() {

  // 1 frame data
  syslog->info(DEBUG_COMM, liveData->responseRow);

  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    liveData->responseRowMerged += liveData->responseRow.substring(2);
  } else if (liveData->responseRow.length() >= 4) {
    liveData->responseRowMerged += liveData->responseRow;
  }
}
