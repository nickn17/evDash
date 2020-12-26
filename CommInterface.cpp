#include "CommInterface.h"
#include "BoardInterface.h"
//#include "CarInterface.h"
#include "LiveData.h"

/**
 * 
 */
void CommInterface::initComm(LiveData* pLiveData, BoardInterface* pBoard) {
    
    liveData = pLiveData;   
    board = pBoard; 
    response = "";  
}

/**
 * Main loop
 */
void CommInterface::mainLoop() {
  
  // Send command from TTY to OBD2
  if (Serial.available()) {
    ch = Serial.read();
    if (ch == '\r' || ch == '\n') {
      board->customConsoleCommand(response);
      response = response + ch;
      Serial.println(response);
      executeCommand(response);
      response = "";
    } else {
      response = response + ch;
    }
  }

  // Drop ChargingOn when status was not updated for more than 10 seconds
  if(liveData->params.currentTime - liveData->params.lastChargingOnTime > 10 && liveData->params.chargingOn) {
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

  // Restart loop with AT commands
  if (liveData->commandQueueIndex >= liveData->commandQueueCount) {
    liveData->commandQueueIndex = liveData->commandQueueLoopFrom;
    board->redrawScreen();
  
    // log every queue loop (temp) TODO and seconds interval
    liveData->params.sdcardCanNotify = true;
  }

  // Send AT command to obd
  liveData->commandRequest = liveData->commandQueue[liveData->commandQueueIndex];
  if (liveData->commandRequest.startsWith("ATSH")) {
    liveData->currentAtshRequest = liveData->commandRequest;
  }

  Serial.print(">>> ");
  Serial.println(liveData->commandRequest);
  liveData->responseRowMerged = "";  
  executeCommand(liveData->commandRequest);
  liveData->commandQueueIndex++;
}

/**
  Parse result from OBD, merge frames to sigle response
 */
bool CommInterface::parseResponse() {
  
  // 1 frame data
  Serial.println(liveData->responseRow);

  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    liveData->responseRowMerged += liveData->responseRow.substring(2);
  }
}
