#pragma once

#include "LiveData.h"
//#include "BoardInterface.h"

class BoardInterface; // Forward declaration

class CommInterface {
  
  protected:
    LiveData* liveData;   
    BoardInterface* board;   
    char ch;
    String response;
    time_t lastDataSent;
    uint8_t connectAttempts = 3;
    // CAN response comparer
    int8_t canComparerRecordIndex = -1; // 0..3, -1 (disabled recording) 
    uint32_t canComparerRecordQueueLoop; // Request to record specified params.queueLoopCounter
    String canComparerData[4] = {"", "", "", ""}; 
  public:
    void initComm(LiveData* pLiveData, BoardInterface* pBoard);
    // Basic
    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    uint8_t checkConnectAttempts();
    virtual void scanDevices() = 0;
    virtual void mainLoop();
    virtual void executeCommand(String cmd) = 0;
    // Command queue processing
    bool doNextQueueCommand();    
    bool parseResponse();
    void parseRowMerged();
    // CAN response comparer
    void recordLoop(int8_t dataIndex);
    void compareCanRecords();
    // CAN car control
    // Please don't use for BLE adapters due to security
    virtual void sendPID(const uint32_t pid, const String& cmd);
    virtual uint8_t receivePID();
};
