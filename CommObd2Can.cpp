#include "CommObd2CAN.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include <mcp_CAN.h>

//#include <string.h>

/**
   Connect CAN adapter
*/
void CommObd2Can::connectDevice() {

  syslog->println("CAN connectDevice");

  //CAN = new MCP_CAN(pinCanCs); // todo: remove if smart pointer is ok
  CAN.reset(new MCP_CAN(pinCanCs)); // smart pointer so it's automatically cleaned when out of context and also free to re-init
  if (CAN == nullptr) {
    syslog->println("Error: Not enough memory to instantiate CAN class");
    syslog->println("init_can() failed");
    return;
  }

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN->begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    syslog->println("MCP2515 Initialized Successfully!");
    board->displayMessage(" > CAN init OK", "");
  } else {
    syslog->println("Error Initializing MCP2515...");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  if (liveData->settings.carType == CAR_BMW_I3_2014) {
    //initialise mask and filter to allow only receipt of 0x7xx CAN IDs
    CAN->init_Mask(0, 0, 0x07000000);              // Init first mask...
    CAN->init_Mask(1, 0, 0x07000000);              // Init second mask...
    for (uint8_t i = 0; i < 6; ++i) {
      CAN->init_Filt(i, 0, 0x06000000);           //Init filters
    }
  }

  if (MCP2515_OK != CAN->setMode(MCP_NORMAL)) {  // Set operation mode to normal so the MCP2515 sends acks to received data.
    syslog->println("Error: CAN->setMode(MCP_NORMAL) failed");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  pinMode(pinCanInt, INPUT);                    // Configuring pin for /INT input

  // Serve first command (ATZ)
  liveData->commConnected = true;  
  doNextQueueCommand();

  syslog->println("init_can() done");
}

/**
   Disconnect device
*/
void CommObd2Can::disconnectDevice() {

  liveData->commConnected = false;  
  syslog->println("COMM disconnectDevice");
}

/**
   Scan device list, from menu
*/
void CommObd2Can::scanDevices() {

  syslog->println("COMM scanDevices");
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
    for (uint16_t i = 0; i < 1000; i++) {
      receivePID();
      if (rxRemaining <= 2)
        break;
      delay(1);
      // apply timeout for next frames loop too
      if (lastDataSent != 0 && (unsigned long)(millis() - lastDataSent) > 100) {
        syslog->print("CAN execution timeout (multiframe message).\n");
        break;
      }
    }
    // Process incomplette messages
    if (liveData->responseRowMerged.length() > 7) {
      processMergedResponse();
      return;
    }
  }
  if (lastDataSent != 0 && (unsigned long)(millis() - lastDataSent) > 100) {
    syslog->print("CAN execution timeout. Continue with next command.\n");
    liveData->canSendNextAtCommand = true;
    return;
  }
}

/**
   Send command to CAN bus
*/
void CommObd2Can::executeCommand(String cmd) {

  syslog->print("executeCommand ");
  syslog->println(cmd);

  if (cmd.equals("") || cmd.startsWith("AT")) { // skip AT commands as not used by direct CAN connection
    lastDataSent = 0;
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

  if (liveData->settings.carType == CAR_BMW_I3_2014)
  {
    struct Packet_t
    {
      uint8_t startChar;
      uint8_t length;
      uint8_t data[6];
    };
    
    Packet_t* pPacket = (Packet_t*)txBuf;

    if (cmd == "22402B" || cmd == "22F101") {
      pPacket->startChar = txStartChar = 0x12;
    } else {
      pPacket->startChar = txStartChar = 0x07;
    }
    
    
    pPacket->length = cmd.length() / 2;
    
    for (uint8_t i = 0; i < sizeof(pPacket->data); i++) {
      tmpStr = cmd;
      tmpStr = tmpStr.substring(i * 2, ((i + 1) * 2));
      if (tmpStr != "") {
        pPacket->data[i] = liveData->hexToDec(tmpStr, 1, false);
      }
    }
  }
  else
  {
    txBuf[0] = cmd.length() / 2;
    
    for (uint8_t i = 0; i < 7; i++) {
      tmpStr = cmd;
      tmpStr = tmpStr.substring(i * 2, ((i + 1) * 2));
      if (tmpStr != "") {
        txBuf[i + 1] = liveData->hexToDec(tmpStr, 1, false);
      }
    }
  }

  lastPid = pid;
  const uint8_t sndStat = CAN->sendMsgBuf(pid, 0, 8, txBuf); // 11 bit
  //  uint8_t sndStat = CAN->sendMsgBuf(0x7e4, 1, 8, tmp); // 29 bit extended frame
  if (sndStat == CAN_OK)  {
    syslog->print("SENT ");
    lastDataSent = millis();
  }   else {
    syslog->print("Error sending PID ");
  }
  syslog->print(pid);
  for (uint8_t i = 0; i < 8; i++) {
    sprintf(msgString, " 0x%.2X", txBuf[i]);
    syslog->print(msgString);
  }
  syslog->println("");
}

/**
   sendFlowControlFrame
*/
void CommObd2Can::sendFlowControlFrame() {

  uint8_t txBuf[8] = { 0x30, requestFramesCount /*request count*/, 20 /*ms between frames*/ , 0, 0, 0, 0, 0 };
  
  // insert 0x07 into beginning for BMW i3
  if (liveData->settings.carType == CAR_BMW_I3_2014) {
    memmove(txBuf + 1, txBuf, 7);
    txBuf[0] = txStartChar;
  }
    
  const uint8_t sndStat = CAN->sendMsgBuf(lastPid, 0, 8, txBuf); // 11 bit
  if (sndStat == CAN_OK)  {
    syslog->print("Flow control frame sent ");
  }   else {
    syslog->print("Error sending flow control frame ");
  }
  syslog->print(lastPid);
  for (auto txByte : txBuf) {
    sprintf(msgString, " 0x%.2X", txByte);
    syslog->print(msgString);
  }
  syslog->println("");
}

/**
   Receive PID
*/
uint8_t CommObd2Can::receivePID() {

  if (!digitalRead(pinCanInt))                        // If CAN0_INT pin is low, read receive buffer
  {
    lastDataSent = millis();
    syslog->print(" CAN READ ");
    CAN->readMsgBuf(&rxId, &rxLen, rxBuf);      // Read data: len = data length, buf = data byte(s)
//    mockupReceiveCanBuf(&rxId, &rxLen, rxBuf);
    
    if ((rxId & 0x80000000) == 0x80000000)    // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), rxLen);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, rxLen);

    syslog->print(msgString);

    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      syslog->print(msgString);
    } else {
      for (uint8_t i = 0; i < rxLen; i++) {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        syslog->print(msgString);
      }
    }

    // Check if this packet shall be discarded due to its length. 
    // If liveData->expectedPacketLength is set to 0, accept any length.
    if(liveData->expectedMinimalPacketLength != 0 && rxLen < liveData->expectedMinimalPacketLength) {
      syslog->println(" [Ignored packet]");
      return 0xff;
    }

    // Filter received messages (Ioniq only)
    if(liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018) {
      long unsigned int atsh_response = liveData->hexToDec(liveData->currentAtshRequest.substring(4), 2, false) + 8;

      if(rxId != atsh_response) {
        syslog->println(" [Filtered packet]");
        return 0xff;
      }
    }
    
    syslog->println();
    processFrameBytes();
    //processFrame();
  } else {
    //syslog->println(" CAN NOT READ ");
    return 0xff;
  }

  return rxBuf[0 + liveData->rxBuffOffset];
}

static void printHexBuffer(uint8_t* pData, const uint16_t length, const bool bAddNewLine)
{
  char str[8] = { 0 };
  
  for (uint8_t i = 0; i < length; i++) {
    sprintf(str, " 0x%.2X", pData[i]);
    syslog->print(str);
  }
  
  if (bAddNewLine) {
    syslog->println();
  }
}

static void buffer2string(String& out_targetString, uint8_t* in_pBuffer, const uint16_t in_length)
{
  char str[8] = { 0 };
  
  for (uint16_t i = 0; i < in_length; i++) {
    sprintf(str, "%.2X", in_pBuffer[i]);
    out_targetString += str;  
  }
  
  
}

CommObd2Can::enFrame_t CommObd2Can::getFrameType(const uint8_t firstByte) {
  const uint8_t frameType = (firstByte & 0xf0) >> 4; // frame type is in bits 7 to 4
  switch(frameType) {
  case 0:
    return enFrame_t::single;
  case 1:
    return enFrame_t::first;
  case 2:
    return enFrame_t::consecutive;
  default:
    return enFrame_t::unknown;
  }
}
  


/**
   Process can frame on byte level
   https://en.wikipedia.org/wiki/ISO_15765-2
 */
bool CommObd2Can::processFrameBytes() {
  
  uint8_t* pDataStart = rxBuf + liveData->rxBuffOffset; // set pointer to data start based on specific offset of car
  const auto frameType = getFrameType(*pDataStart);
  const uint8_t frameLenght = rxLen - liveData->rxBuffOffset;
  
  switch (frameType) {
  case enFrame_t::single: // Single frame
    {
      struct SingleFrame_t
      {
        uint8_t size : 4;
        uint8_t frameType : 4;
        uint8_t pData[];
      };
      
      SingleFrame_t* pSingleFrame = (SingleFrame_t*)pDataStart;
      mergedData.assign(pSingleFrame->pData, pSingleFrame->pData + pSingleFrame->size);
      
      rxRemaining = 0;
      
      //syslog->print("----Processing SingleFrame payload: "); printHexBuffer(pSingleFrame->pData, pSingleFrame->size, true);
      
      // single frame - process directly
      buffer2string(liveData->responseRowMerged, mergedData.data(), mergedData.size());
      liveData->vResponseRowMerged.assign(mergedData.begin(), mergedData.end());
      processMergedResponse();
    }
    break;
    
  case enFrame_t::first: // First frame
    {
      struct FirstFrame_t
      {
        uint8_t sizeMSB : 4;
        uint8_t frameType : 4;
        uint8_t sizeLSB : 8;
        uint8_t pData[];
    
        uint16_t lengthOfFullPacket() { return (256 * sizeMSB) + sizeLSB; }
    
      };
      
      FirstFrame_t* pFirstFrame = (FirstFrame_t*)pDataStart;
      
      rxRemaining = pFirstFrame->lengthOfFullPacket(); // length of complete data
      
      mergedData.clear();
      dataRows.clear();
      
      const uint8_t framePayloadSize = frameLenght - sizeof(FirstFrame_t);    // remove one byte of header
      dataRows[0].assign(pFirstFrame->pData, pFirstFrame->pData + framePayloadSize);
      rxRemaining -= framePayloadSize;
      
      //syslog->print("----Processing FirstFrame payload: "); printHexBuffer(pFirstFrame->pData, framePayloadSize, true);
    }
    break;
    
  case enFrame_t::consecutive: // Consecutive frame
    {
      struct ConsecutiveFrame_t
      {
        uint8_t index : 4;
        uint8_t frameType : 4;
        uint8_t pData[];
      };
      
      const uint8_t structSize = sizeof(ConsecutiveFrame_t);
      //syslog->print("[debug] sizeof(ConsecutiveFrame_t) is expected to be 1 and it's "); syslog->println(structSize);
      
      ConsecutiveFrame_t* pConseqFrame = (ConsecutiveFrame_t*)pDataStart;
      const uint8_t framePayloadSize = frameLenght - sizeof(ConsecutiveFrame_t);  // remove one byte of header
      dataRows[pConseqFrame->index].assign(pConseqFrame->pData, pConseqFrame->pData + framePayloadSize);
      rxRemaining -= framePayloadSize;
      
      //syslog->print("----Processing ConsecFrame payload: "); printHexBuffer(pConseqFrame->pData, framePayloadSize, true);
    }
    break;
    
  default:
    syslog->print("Unknown frame type within CommObd2Can::processFrameBytes(): "); syslog->println((uint8_t)frameType);
    return false;
    break;
  } // \switch (frameType)
  
  // Merge data if all data was received
  if (rxRemaining <= 0) {
    // multiple frames and no data remaining - merge everything to single packet
    for (int i = 0; i < dataRows.size(); i++) {
      //syslog->print("------merging packet index ");
      //syslog->print(i);
      //syslog->print(" with length ");
      //syslog->println(dataRows[i].size());
      
      mergedData.insert(mergedData.end(), dataRows[i].begin(), dataRows[i].end());
    }
    
    buffer2string(liveData->responseRowMerged, mergedData.data(), mergedData.size()); // output for string parsing
    liveData->vResponseRowMerged.assign(mergedData.begin(), mergedData.end());   // output for binary parsing
    processMergedResponse();
  }
  
  return true;
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
      rxRemaining = (rxBuf[1] & 0x0f);
      requestFramesCount = 0;
      break;
    // First frame
    case 1:
      rxRemaining = ((rxBuf[0] & 0x0f) << 8) + rxBuf[1];
      requestFramesCount = ceil((rxRemaining - 6) / 7.0);
      liveData->responseRowMerged = "";
      for (uint16_t i = 0; i < rxRemaining - 1; i++)
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

  syslog->print("> frametype:");
  syslog->print(frameType);
  syslog->print(", r: ");
  syslog->print(rxRemaining);
  syslog->print("   ");

  for (uint8_t i = start; i < rxLen; i++) {
    sprintf(msgString, "%.2X", rxBuf[i]);
    liveData->responseRow += msgString;
    rxRemaining--;
  }

  syslog->print(", r: ");
  syslog->print(rxRemaining);
  syslog->println("   ");

  //parseResponse();
  // We need to sort frames
  // 1 frame data
  syslog->println(liveData->responseRow);
  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':') {
    //liveData->responseRowMerged += liveData->responseRow.substring(2);
    uint8_t rowNo = liveData->hexToDec(liveData->responseRow.substring(0, 1), 1, false);
    uint16_t startPos = (rowNo * 14) - ((rowNo > 0) ? 2 : 0);
    uint16_t endPos = ((rowNo + 1) * 14) - ((rowNo > 0) ? 2 : 0);
    liveData->responseRowMerged = liveData->responseRowMerged.substring(0, startPos) + liveData->responseRow.substring(2) + liveData->responseRowMerged.substring(endPos);
    syslog->println(liveData->responseRowMerged);
  }

  // Send response to board module
  if (rxRemaining <= 2) {
    processMergedResponse();
    return false;
  }

  return true;
}

/**
   processMergedResponse
*/
void CommObd2Can::processMergedResponse() {
  syslog->print("merged:");
  syslog->println(liveData->responseRowMerged);
  board->parseRowMerged();
  liveData->responseRowMerged = "";
  liveData->canSendNextAtCommand = true;
}
