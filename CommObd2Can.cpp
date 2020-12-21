#include "CommObd2CAN.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include <mcp_CAN.h>

/**
   Connect CAN adapter
*/
void CommObd2Can::connectDevice() {

  Serial.println("CAN connectDevice");

  //CAN = new MCP_CAN(pinCanCs); // todo: remove if smart pointer is ok
  CAN.reset(new MCP_CAN(pinCanCs)); // smart pointer so it's automatically cleaned when out of context and also free to re-init
  if (CAN == nullptr) {
    Serial.println("Error: Not enough memory to instantiate CAN class");
    Serial.println("init_can() failed");
    return;
  }

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN->begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
    board->displayMessage(" > CAN init OK", "");
  } else {
    Serial.println("Error Initializing MCP2515...");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  if (MCP2515_OK != CAN->setMode(MCP_NORMAL)) {  // Set operation mode to normal so the MCP2515 sends acks to received data.
    Serial.println("Error: CAN->setMode(MCP_NORMAL) failed");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  pinMode(pinCanInt, INPUT);                    // Configuring pin for /INT input

  // Serve first command (ATZ)
  doNextQueueCommand();

  Serial.println("init_can() done");
}

/**
   Disconnect device
*/
void CommObd2Can::disconnectDevice() {

  Serial.println("COMM disconnectDevice");
}

/**
   Scan device list, from menu
*/
void CommObd2Can::scanDevices() {

  Serial.println("COMM scanDevices");
}

/**
   Main loop
*/
void CommObd2Can::mainLoop() {

  CommInterface::mainLoop();

  // Read data
  const uint8_t firstByte = receivePID();
  if ((firstByte & 0xf0) == 0x10) { // First frame, request another
    sendFlowControlFrame();
    delay(10);
    for (uint8_t i = 0; i < 50; i++) {
      receivePID();
      if (rxRemaining <= 0)
        break;
      delay(10);
    }
    delay(30);
  }
  Serial.println(liveData->params.currentTime );
  if (lastDataSent != 0 && (unsigned long)(millis() - lastDataSent) > 500) {
    Serial.print("CAN execution timeout. Continue with next command.");
    liveData->canSendNextAtCommand = true;
    return;
  }
}

/**
   Send command to CAN bus
*/
void CommObd2Can::executeCommand(String cmd) {

  Serial.print("executeCommand ");
  Serial.println(cmd);

  if (cmd.equals("") || cmd.startsWith("AT")) { // skip AT commands as not used by direct CAN connection
    liveData->canSendNextAtCommand = true;
    return;
  }

  // Send command
  liveData->responseRowMerged = "";
  liveData->currentAtshRequest.replace(" ", ""); // remove possible spaces
  String atsh = "0" + liveData->currentAtshRequest.substring(4); // remove ATSH
  cmd.replace(" ", ""); // remove possible spaces
  sendPID(liveData->hexToDec(atsh, 2, false), cmd);
   delay(40);
}

/**
   Send PID
   remark: parameter cmd as const reference to aviod copying
*/
void CommObd2Can::sendPID(const uint16_t pid, const String& cmd) {

  uint8_t txBuf[8] = { 0 }; // init with zeroes
  String tmpStr;

  txBuf[0] = cmd.length() / 2;

  for (uint8_t i = 0; i < 7; i++) {
    tmpStr = cmd;
    tmpStr = tmpStr.substring(i * 2, ((i + 1) * 2));
    if (tmpStr != "") {
      txBuf[i + 1] = liveData->hexToDec(tmpStr, 1, false);
    }
  }

  lastPid = pid;
  const uint8_t sndStat = CAN->sendMsgBuf(pid, 0, 8, txBuf); // 11 bit
  //  uint8_t sndStat = CAN->sendMsgBuf(0x7e4, 1, 8, tmp); // 29 bit extended frame
  if (sndStat == CAN_OK)  {
    Serial.print("SENT ");
  }   else {
    Serial.print("Error sending PID ");
  }
  Serial.print(pid);
  for (uint8_t i = 0; i < 8; i++) {
    sprintf(msgString, " 0x%.2X", txBuf[i]);
    Serial.print(msgString);
  }
  Serial.println("");
  lastDataSent = millis();
}

/**
   sendFlowControlFrame
*/
void CommObd2Can::sendFlowControlFrame() {

  uint8_t txBuf[8] = { 0x30, 0, 0, 0, 0, 0, 0, 0 };
  const uint8_t sndStat = CAN->sendMsgBuf(lastPid, 0, 8, txBuf); // 11 bit
  if (sndStat == CAN_OK)  {
    Serial.print("Flow control frame sent ");
  }   else {
    Serial.print("Error sending flow control frame ");
  }
  Serial.print(lastPid);
  for (auto txByte : txBuf) {
    sprintf(msgString, " 0x%.2X", txByte);
    Serial.print(msgString);
  }
  Serial.println("");
}

/**
   Receive PID
*/
uint8_t CommObd2Can::receivePID() {

  if (!digitalRead(pinCanInt))                        // If CAN0_INT pin is low, read receive buffer
  {
    Serial.print(" CAN READ ");
    CAN->readMsgBuf(&rxId, &rxLen, rxBuf);      // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000)    // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), rxLen);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, rxLen);

    Serial.print(msgString);

    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for (uint8_t i = 0; i < rxLen; i++) {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
    }

    Serial.println();
    processFrame();
  } else {
    //Serial.println(" CAN NOT READ ");
    return 0xff;
  }

  return rxBuf[0];
}

/**
   Process can frame
   https://en.wikipedia.org/wiki/ISO_15765-2
*/
bool CommObd2Can::processFrame() {

  const uint8_t frameType = (rxBuf[0] & 0xf0) >> 4;
  uint8_t start = 1; // Single and Consecutive starts with pos 1
  uint8_t index = 0; // 0 - f

  liveData->responseRow = "";
  switch (frameType) {
    // Single frame
    case 0:
      rxRemaining = (rxBuf[1] & 0x0f) + 1;
      break;
    // First frame
    case 1:
      rxRemaining = ((rxBuf[0] & 0x0f) << 8) + rxBuf[1] + 1;
      liveData->responseRowMerged = "";
      for (uint16_t i = 0; i < 62; i++)
        liveData->responseRowMerged += "00";
      liveData->responseRow = "0:";
      start = 2;
      break;
    // Consecutive frames
    case 2:
      index = (rxBuf[0] & 0x0f);
      sprintf(msgString, "%.1X:", index);
      liveData->responseRow = msgString; // convert 0..15 to ascii 0..F);
      break;
  }

  Serial.print("> frametype:");
  Serial.print(frameType);
  Serial.print(", r: ");
  Serial.print(rxRemaining);
  Serial.print("   ");

  for (uint8_t i = start; i < rxLen; i++) {
    sprintf(msgString, "%.2X", rxBuf[i]);
    liveData->responseRow += msgString;
    rxRemaining--;
  }

  Serial.print(", r: ");
  Serial.print(rxRemaining);
  Serial.println("   ");

  //parseResponse();
  // We need to sort frames
  // 1 frame data
  Serial.println(liveData->responseRow);
  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    //liveData->responseRowMerged += liveData->responseRow.substring(2);
    uint8_t rowNo = liveData->hexToDec(liveData->responseRow.substring(0, 1), 1, false);
    uint16_t startPos = (rowNo * 14) - ((rowNo > 0) ? 2 : 0);
    uint16_t endPos = ((rowNo + 1) * 14) - ((rowNo > 0) ? 2 : 0);
    liveData->responseRowMerged = liveData->responseRowMerged.substring(0, startPos) + liveData->responseRow.substring(2) + liveData->responseRowMerged.substring(endPos);
    Serial.println(liveData->responseRowMerged);
  }

  // Send command to board module (obd2 simulation)
  if (rxRemaining <= 0) {
    Serial.print("merged:");
    Serial.println(liveData->responseRowMerged);
    board->parseRowMerged();
    liveData->responseRowMerged = "";
    liveData->canSendNextAtCommand = true;
    return false;
  }

  return true;
}
