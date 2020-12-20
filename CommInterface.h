#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H

#include "LiveData.h"
//#include "BoardInterface.h"

class BoardInterface; // Forward declaration

class CommInterface {
  
  protected:
    LiveData* liveData;   
    BoardInterface* board;   
    char ch;
    String line;
  public:
    void initComm(LiveData* pLiveData, BoardInterface* pBoard);
    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual void scanDevices() = 0;
    virtual void mainLoop() = 0;
};

#endif // COMMINTERFACE_H
