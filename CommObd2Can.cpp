#include "CommObd2CAN.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include <mcp_CAN.h>

/**
   Connect CAN adapter
*/
void CommObd2Can::connectDevice() {

  Serial.println("CAN connectDevice");

  CAN = new MCP_CAN(pinCanCs);

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN->begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
    board->displayMessage(" > CAN init OK", "");
  } else {
    Serial.println("Error Initializing MCP2515...");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  CAN->setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.
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

  // Receive
  /*    byte b = receivePID(0);
    if (b == 0x10) {
    Serial.println("CONTINUE");
    sendPID(0x01, 0x00);
    delay(10);
    for (byte i = 0; i < 20; i++) {
      receivePID(0);
      delay(10);
    }
  */
}

/**
   Send command to CAN bus
*/
void CommObd2Can::executeCommand(String cmd) {

  if (cmd.startsWith("AT")) {
   liveData->canSendNextAtCommand = true;
    return;
  }

  // pid
  /*String atsh = liveData->currentAtshRequest;
  atsh = atsh.replace(" ", "");
  atsh = atsh.substring(4); // remove ATSH
  String tmpCmd = cmd;
  tmpCmd = tmpCmd.replace(" ", "");
  sendPID(liveData->hexToDec(liveData->currentAtshRequest, 2, false), tmpCmd);*/
  //delay(delayTxRx);
}

/**
   Send PID
*/
void CommObd2Can::sendPID(uint16_t pid, String cmd) {

  unsigned char tmp[8];
  String tmpStr;

/*  tmp[0] = cmd.length() / 2;
  for (byte i = 1; i < 8; i++) {
    tmp[i] = 0x00;
  }
  for (byte i = 0; i < 7; i++) {
    tmpStr = cmd;
    tmpStr = tmpStr.substring(i * 2, ((i + 1) * 2);
    if (tmpStr != "") {
    tmp[i + 1] = liveData->hexToDec(tmpString, 1, false);
    }
  }

  byte sndStat = CAN->sendMsgBuf(pid, 0, 8, tmp); // 11 bit
  //  byte sndStat = CAN->sendMsgBuf(0x7e4, 1, 8, tmp); // 29 bit extended frame
  if (sndStat == CAN_OK)  {
    Serial.print("SENT ");
    for (byte i = 0; i < 8; i++) {
      sprintf(msgString, " 0x%.2X", tmp[i]);
      Serial.print(msgString);
    }
    Serial.println("");
  }   else {
    Serial.print("Error Sending PID2 0x");
  }*/
}

/**
   sendFlowControlFrame
*/
void CommObd2Can::sendFlowControlFrame() {

  unsigned char tmp[8];
/*
  Serial.println("Flow control frame");
  tmp[0] = 0x30;
  for (byte i = 1; i < 8; i++) {
    tmp[i] = 0x00;
  }

  byte sndStat = CAN->sendMsgBuf(pid, 0, 8, tmp); // 11 bit
  //  byte sndStat = CAN->sendMsgBuf(0x7e4, 1, 8, tmp); // 29 bit extended frame
  if (sndStat == CAN_OK)  {
    Serial.print("SENT ");
    for (byte i = 0; i < 8; i++) {
      sprintf(msgString, " 0x%.2X", tmp[i]);
      Serial.print(msgString);
    }
    Serial.println("");
  }   else {
    Serial.print("Error Sending PID2 0x");
  }*/
}

/**
 * Receive PID
 */
/*byte CommObd2Can::receivePID() {

  if (!digitalRead(CAN_INT))                        // If CAN0_INT pin is low, read receive buffer
  {
    Serial.print(" CAN READ ");
    CAN->readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000)    // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

    Serial.print(msgString);

    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      for (byte i = 0; i < len; i++) {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        Serial.print(msgString);
      }
    }

    Serial.println();
  } else {
    Serial.println(" CAN NOT READ ");
  }
  
  return rxBuf[0];
}*/
