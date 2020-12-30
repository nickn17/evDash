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
};
