#pragma once

#include "LiveData.h"

class BoardInterface; // Forward declaration

class CommInterface
{
protected:
  LiveData *liveData;
  BoardInterface *board;
  char ch;
  String response;
  time_t lastDataSent;
  int8_t connectAttempts = 3;
  String connectStatus = "";
  int8_t canComparerRecordIndex = -1;
  uint32_t canComparerRecordQueueLoop;
  String canComparerData[4] = {"", "", "", ""};

public:
  void initComm(LiveData *pLiveData, BoardInterface *pBoard);
  virtual void connectDevice() = 0;
  virtual void disconnectDevice() = 0;
  uint8_t checkConnectAttempts();
  int8_t getConnectAttempts();
  String getConnectStatus();
  virtual void scanDevices() = 0;
  virtual void mainLoop();
  virtual void executeCommand(String cmd) = 0;
  bool doNextQueueCommand();
  bool parseResponse();
  void parseRowMerged();
  void recordLoop(int8_t dataIndex);
  void compareCanRecords();
  virtual void sendPID(const uint32_t pid, const String &cmd);
  virtual uint8_t receivePID();
};
