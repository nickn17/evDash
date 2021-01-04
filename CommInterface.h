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
    // 
    int8_t canComparerRecordIndex = -1; // 0..3, -1 (disabled recording) 
    uint32_t canComparerRecordQueueLoop; // Request to record specified params.queueLoopCounter
    String canComparerData[4] = {"", "", "", ""}; 
  public:
    void initComm(LiveData* pLiveData, BoardInterface* pBoard);
    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual void scanDevices() = 0;
    virtual void mainLoop();
    virtual void executeCommand(String cmd) = 0;
    // 
    bool doNextQueueCommand();    
    bool parseResponse();
    void parseRowMerged();
    // CAN comparer
    void recordLoop(int8_t dataIndex);
    void compareCanRecords();
};
