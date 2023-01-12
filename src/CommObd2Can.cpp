#include "CommObd2Can.h"
#include "BoardInterface.h"
#include "LiveData.h"
#include <MCP_CAN_lib/mcp_can.h>

// #include <string.h>

/**
   Connect CAN adapter
*/
void CommObd2Can::connectDevice()
{
  connectAttempts--;

  syslog->println("CAN connectDevice");

  // CAN = new MCP_CAN(pinCanCs); // todo: remove if smart pointer is ok
  CAN.reset(new MCP_CAN(/*&SPI, */ pinCanCs)); // smart pointer so it's automatically cleaned when out of context and also free to re-init
  if (CAN == nullptr)
  {
    syslog->println("Error: Not enough memory to instantiate CAN class");
    syslog->println("init_can() failed");
    return;
  }

  // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if (CAN->begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
  {
    syslog->println("MCP2515 Initialized Successfully!");
    board->displayMessage(" > CAN init OK", "");
    connectAttempts = 3;
  }
  else
  {
    syslog->println("Error Initializing MCP2515...");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  if (liveData->settings.carType == CAR_BMW_I3_2014)
  {
    // initialise mask and filter to allow only receipt of 0x7xx CAN IDs
    CAN->init_Mask(0, 0, 0x07000000); // Init first mask...
    CAN->init_Mask(1, 0, 0x07000000); // Init second mask...
    for (uint8_t i = 0; i < 6; ++i)
    {
      CAN->init_Filt(i, 0, 0x06000000); // Init filters
    }
  }

  if (MCP2515_OK != CAN->setMode(MCP_NORMAL))
  { // Set operation mode to normal so the MCP2515 sends acks to received data.
    syslog->println("Error: CAN->setMode(MCP_NORMAL) failed");
    board->displayMessage(" > CAN init failed", "");
    return;
  }

  pinMode(pinCanInt, INPUT); // Configuring pin for /INT input

  // Serve first command (ATZ)
  liveData->commConnected = true;
  liveData->commandQueueIndex = 0;
  doNextQueueCommand();

  syslog->println("init_can() done");
}

/**
   Disconnect device
*/
void CommObd2Can::disconnectDevice()
{

  liveData->commConnected = false;
  // CAN->setMode(MCP_SLEEP);
  syslog->println("COMM disconnectDevice");
}

/**
   Scan device list, from menu
*/
void CommObd2Can::scanDevices()
{

  syslog->println("COMM scanDevices");
}

/**
   Main loop
*/
void CommObd2Can::mainLoop()
{
  CommInterface::mainLoop();

  if (liveData->params.stopCommandQueue)
    return;

  // if delay between commands is defined, check if this delay is not expired
  if (liveData->delayBetweenCommandsMs != 0)
  {
    if (bResponseProcessed && (unsigned long)(millis() - lastDataSent) > liveData->delayBetweenCommandsMs)
    {
      bResponseProcessed = false;
      liveData->canSendNextAtCommand = true;
      return;
    }
  }

  // Read data
  const uint8_t firstByte = receivePID();
  if ((firstByte & 0xf0) == 0x10)
  { // First frame, request another
    sendFlowControlFrame();
    delay(10);
    for (uint16_t i = 0; i < 1000; i++)
    {
      receivePID();
      if (rxRemaining <= 2)
        break;
      delay(1);
      // apply timeout for next frames loop too
      if (lastDataSent != 0 && (unsigned long)(millis() - lastDataSent) > liveData->rxTimeoutMs)
      {
        syslog->info(DEBUG_COMM, "CAN execution timeout (multiframe message).");
        break;
      }
    }
    // Process incomplette messages
    if (liveData->responseRowMerged.length() > 7)
    {
      processMergedResponse();
      return;
    }
  }
  if (lastDataSent != 0 && (unsigned long)(millis() - lastDataSent) > liveData->rxTimeoutMs)
  {
    syslog->info(DEBUG_COMM, "CAN execution timeout. Continue with next command.");
    liveData->canSendNextAtCommand = true;
    return;
  }
}

/**
   Send command to CAN bus
*/
void CommObd2Can::executeCommand(String cmd)
{

  syslog->infoNolf(DEBUG_COMM, "executeCommand ");
  syslog->info(DEBUG_COMM, cmd);

  if (cmd.equals("") || cmd.startsWith("AT"))
  { // skip AT commands as not used by direct CAN connection
    lastDataSent = 0;
    liveData->canSendNextAtCommand = true;
    return;
  }

  // Send command
  liveData->responseRowMerged = "";
  liveData->currentAtshRequest.replace(" ", "");                 // remove possible spaces
  String atsh = "0" + liveData->currentAtshRequest.substring(4); // remove ATSH
  cmd.replace(" ", "");                                          // remove possible spaces
  sendPID(liveData->hexToDec(atsh, 4, false), cmd);

  delay(40);
}

/**
   Send PID
   remark: parameter cmd as const reference to aviod copying
*/
void CommObd2Can::sendPID(const uint32_t pid, const String &cmd)
{

  uint8_t txBuf[8] = {0}; // init with zeroes
  String tmpStr;

  if (liveData->bAdditionalStartingChar)
  {
    struct Packet_t
    {
      uint8_t startChar;
      uint8_t length;
      uint8_t payload[6];
    };

    Packet_t *pPacket = (Packet_t *)txBuf;
    pPacket->startChar = liveData->commandStartChar; // todo: handle similar way as cmd input param?
    pPacket->length = cmd.length() / 2;

    for (uint8_t i = 0; i < sizeof(pPacket->payload); i++)
    {
      tmpStr = cmd;
      tmpStr = tmpStr.substring(i * 2, ((i + 1) * 2));
      if (tmpStr != "")
      {
        pPacket->payload[i] = liveData->hexToDec(tmpStr, 1, false);
      }
    }
  }
  else
  {
    struct Packet_t
    {
      uint8_t length;
      uint8_t payload[7];
    };

    Packet_t *pPacket = (Packet_t *)txBuf;
    pPacket->length = cmd.length() / 2;

    for (uint8_t i = 0; i < sizeof(pPacket->payload); i++)
    {
      tmpStr = cmd;
      tmpStr = tmpStr.substring(i * 2, ((i + 1) * 2));
      if (tmpStr != "")
      {
        pPacket->payload[i] = liveData->hexToDec(tmpStr, 1, false);
      }
    }
  }

  lastPid = pid;
  bResponseProcessed = false;
  // logic to choose from 11-bit or 29-bit by testing if PID is bigger than 4095 (ie bigger than FF)
  byte is29bit = 0;
  if (pid > 4095)
    is29bit = 1;

  const uint8_t sndStat = CAN->sendMsgBuf(pid, is29bit, 8, txBuf); // 11 bit or 29 bit
  if (sndStat == CAN_OK)
  {
    syslog->infoNolf(DEBUG_COMM, "SENT ");
    lastDataSent = millis();
  }
  else
  {
    syslog->infoNolf(DEBUG_COMM, "Error sending PID ");
    lastDataSent = millis();
  }
  syslog->infoNolf(DEBUG_COMM, pid);
  for (uint8_t i = 0; i < 8; i++)
  {
    sprintf(msgString, " 0x%.2X", txBuf[i]);
    syslog->infoNolf(DEBUG_COMM, msgString);
  }
  syslog->info(DEBUG_COMM, "");
}

/**
   sendFlowControlFrame
*/
void CommObd2Can::sendFlowControlFrame()
{

  uint8_t txBuf[8] = {0x30, requestFramesCount /*request count*/, 20 /*ms between frames*/, 0, 0, 0, 0, 0};

  // insert start char if needed
  if (liveData->bAdditionalStartingChar)
  {
    memmove(txBuf + 1, txBuf, 7);
    txBuf[0] = liveData->commandStartChar;
  }
  // logic to choose from 11-bit or 29-bit
  byte is29bit = 0;
  if (lastPid > 4095)
    is29bit = 1;
  const uint8_t sndStat = CAN->sendMsgBuf(lastPid, is29bit, 8, txBuf); // VW:29bit vs others:11 bit
  if (sndStat == CAN_OK)
  {
    syslog->infoNolf(DEBUG_COMM, "Flow control frame sent ");
  }
  else
  {
    syslog->infoNolf(DEBUG_COMM, "Error sending flow control frame ");
  }
  syslog->infoNolf(DEBUG_COMM, lastPid);
  for (auto txByte : txBuf)
  {
    sprintf(msgString, " 0x%.2X", txByte);
    syslog->infoNolf(DEBUG_COMM, msgString);
  }
  syslog->info(DEBUG_COMM, "");
}

/**
   Receive PID
*/
uint8_t CommObd2Can::receivePID()
{

  if (!digitalRead(pinCanInt)) // If CAN0_INT pin is low, read receive buffer
  {
    lastDataSent = millis();
    syslog->infoNolf(DEBUG_COMM, " CAN READ ");
    CAN->readMsgBuf(&rxId, &rxLen, rxBuf); // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), rxLen);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, rxLen);

    syslog->infoNolf(DEBUG_COMM, msgString);

    if ((rxId & 0x40000000) == 0x40000000)
    { // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      syslog->infoNolf(DEBUG_COMM, msgString);
    }
    else
    {
      for (uint8_t i = 0; i < rxLen; i++)
      {
        sprintf(msgString, " 0x%.2X", rxBuf[i]);
        syslog->infoNolf(DEBUG_COMM, msgString);
      }
    }

    // Check if this packet shall be discarded due to its length.
    // If liveData->expectedPacketLength is set to 0, accept any length.
    if (liveData->expectedMinimalPacketLength != 0 && rxLen < liveData->expectedMinimalPacketLength)
    {
      syslog->info(DEBUG_COMM, " [Ignored packet]");
      return 0xff;
    }

    // Filter received messages (Ioniq only)
    if (liveData->settings.carType == CAR_HYUNDAI_IONIQ_2018)
    {
      long unsigned int atsh_response = liveData->hexToDec(liveData->currentAtshRequest.substring(4), 2, false) + 8;

      if (rxId != atsh_response)
      {
        syslog->info(DEBUG_COMM, " [Filtered packet]");
        return 0xff;
      }
    }

    syslog->info(DEBUG_COMM, "");
    processFrameBytes();
    // processFrame();
  }
  else
  {
    // syslog->println(" CAN NOT READ ");
    return 0xff;
  }

  const uint8_t rxBuffOffset = liveData->bAdditionalStartingChar ? 1 : 0;
  return rxBuf[0 + rxBuffOffset]; // return byte containing frame type, which requires removing offset byte
}

void printHexBuffer(uint8_t *pData, const uint16_t length, const bool bAddNewLine)
{
  char str[8] = {0};

  for (uint8_t i = 0; i < length; i++)
  {
    sprintf(str, " 0x%.2X", pData[i]);
    syslog->infoNolf(DEBUG_COMM, str);
  }

  if (bAddNewLine)
  {
    syslog->info(DEBUG_COMM, "");
  }
}

static void buffer2string(String &out_targetString, uint8_t *in_pBuffer, const uint16_t in_length)
{
  char str[8] = {0};

  for (uint16_t i = 0; i < in_length; i++)
  {
    sprintf(str, "%.2X", in_pBuffer[i]);
    out_targetString += str;
  }
}

CommObd2Can::enFrame_t CommObd2Can::getFrameType(const uint8_t firstByte)
{
  const uint8_t frameType = (firstByte & 0xf0) >> 4; // frame type is in bits 7 to 4
  switch (frameType)
  {
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
bool CommObd2Can::processFrameBytes()
{
  const uint8_t rxBuffOffset = liveData->bAdditionalStartingChar ? 1 : 0;
  uint8_t *pDataStart = rxBuf + rxBuffOffset; // set pointer to data start based on specific offset of car
  const auto frameType = getFrameType(*pDataStart);
  const uint8_t frameLenght = rxLen - rxBuffOffset;

  switch (frameType)
  {
  case enFrame_t::single: // Single frame
  {
    struct SingleFrame_t
    {
      uint8_t size : 4;
      uint8_t frameType : 4;
      uint8_t pData[];
    };

    SingleFrame_t *pSingleFrame = (SingleFrame_t *)pDataStart;
    mergedData.assign(pSingleFrame->pData, pSingleFrame->pData + pSingleFrame->size);

    rxRemaining = 0;

    // syslog->print("----Processing SingleFrame payload: "); printHexBuffer(pSingleFrame->pData, pSingleFrame->size, true);

    // single frame - process directly
    buffer2string(liveData->responseRowMerged, mergedData.data(), mergedData.size());
    liveData->vResponseRowMerged.assign(mergedData.begin(), mergedData.end());
    processMergedResponse();

    return true;
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

    FirstFrame_t *pFirstFrame = (FirstFrame_t *)pDataStart;

    rxRemaining = pFirstFrame->lengthOfFullPacket(); // length of complete data

    mergedData.clear();
    dataRows.clear();

    const uint8_t framePayloadSize = frameLenght - sizeof(FirstFrame_t); // remove one byte of header
    dataRows[0].assign(pFirstFrame->pData, pFirstFrame->pData + framePayloadSize);
    rxRemaining -= framePayloadSize;

    // syslog->print("----Processing FirstFrame payload: "); printHexBuffer(pFirstFrame->pData, framePayloadSize, true);
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

    // const uint8_t structSize = sizeof(ConsecutiveFrame_t);
    // syslog->print("[debug] sizeof(ConsecutiveFrame_t) is expected to be 1 and it's "); syslog->println(structSize);

    ConsecutiveFrame_t *pConseqFrame = (ConsecutiveFrame_t *)pDataStart;
    const uint8_t framePayloadSize = frameLenght - sizeof(ConsecutiveFrame_t); // remove one byte of header
    dataRows[pConseqFrame->index].assign(pConseqFrame->pData, pConseqFrame->pData + framePayloadSize);
    rxRemaining -= framePayloadSize;

    // syslog->print("----Processing ConsecFrame payload: "); printHexBuffer(pConseqFrame->pData, framePayloadSize, true);
  }
  break;

  default:
    syslog->infoNolf(DEBUG_COMM, "Unknown frame type within CommObd2Can::processFrameBytes(): ");
    syslog->info(DEBUG_COMM, (uint8_t)frameType);
    return false;
    break;
  } // \switch (frameType)

  // Merge data if all data was received
  if (rxRemaining <= 0)
  {
    // multiple frames and no data remaining - merge everything to single packet
    for (int i = 0; i < dataRows.size(); i++)
    {
      // syslog->print("------merging packet index ");
      // syslog->print(i);
      // syslog->print(" with length ");
      // syslog->println(dataRows[i].size());

      mergedData.insert(mergedData.end(), dataRows[i].begin(), dataRows[i].end());
    }

    buffer2string(liveData->responseRowMerged, mergedData.data(), mergedData.size()); // output for string parsing
    liveData->vResponseRowMerged.assign(mergedData.begin(), mergedData.end());        // output for binary parsing
    processMergedResponse();
  }

  return true;
}

/**
   Process can frame
   https://en.wikipedia.org/wiki/ISO_15765-2
*/
bool CommObd2Can::processFrame()
{

  const uint8_t frameType = (rxBuf[0] & 0xf0) >> 4;
  uint8_t start = 1; // Single and Consecutive starts with pos 1
  uint8_t index = 0; // 0 - f

  liveData->responseRow = "";
  switch (frameType)
  {
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

  syslog->infoNolf(DEBUG_COMM, "> frametype:");
  syslog->infoNolf(DEBUG_COMM, frameType);
  syslog->infoNolf(DEBUG_COMM, ", r: ");
  syslog->infoNolf(DEBUG_COMM, rxRemaining);
  syslog->infoNolf(DEBUG_COMM, "   ");

  for (uint8_t i = start; i < rxLen; i++)
  {
    sprintf(msgString, "%.2X", rxBuf[i]);
    liveData->responseRow += msgString;
    rxRemaining--;
  }

  syslog->infoNolf(DEBUG_COMM, ", r: ");
  syslog->infoNolf(DEBUG_COMM, rxRemaining);
  syslog->info(DEBUG_COMM, "   ");

  // parseResponse();
  //  We need to sort frames
  //  1 frame data
  syslog->info(DEBUG_COMM, liveData->responseRow);
  // Merge frames 0:xxxx 1:yyyy 2:zzzz to single response xxxxyyyyzzzz string
  if (liveData->responseRow.length() >= 2 && liveData->responseRow.charAt(1) == ':')
  {
    // liveData->responseRowMerged += liveData->responseRow.substring(2);
    uint8_t rowNo = liveData->hexToDec(liveData->responseRow.substring(0, 1), 1, false);
    uint16_t startPos = (rowNo * 14) - ((rowNo > 0) ? 2 : 0);
    uint16_t endPos = ((rowNo + 1) * 14) - ((rowNo > 0) ? 2 : 0);
    liveData->responseRowMerged = liveData->responseRowMerged.substring(0, startPos) + liveData->responseRow.substring(2) + liveData->responseRowMerged.substring(endPos);
    syslog->info(DEBUG_COMM, liveData->responseRowMerged);
  }

  // Send response to board module
  if (rxRemaining <= 2)
  {
    processMergedResponse();
    return false;
  }

  return true;
}

/**
   processMergedResponse
*/
void CommObd2Can::processMergedResponse()
{
  syslog->infoNolf(DEBUG_COMM, "merged:");
  syslog->info(DEBUG_COMM, liveData->responseRowMerged);

  // Wait for real response (MEB GPS sometimes return 7F2278 and then real data)
  if (liveData->responseRowMerged == "7F2278")
  {
    liveData->responseRowMerged = "";
    liveData->vResponseRowMerged.clear();
    return;
  }

  parseRowMerged();

  liveData->prevResponseRowMerged = liveData->responseRowMerged;
  liveData->responseRowMerged = "";
  liveData->vResponseRowMerged.clear();
  bResponseProcessed = true; // to allow delay until next message

  if (liveData->delayBetweenCommandsMs == 0)
  {
    liveData->canSendNextAtCommand = true; // allow next command immediately
  }
}
