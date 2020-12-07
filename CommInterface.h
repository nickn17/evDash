#ifndef COMMINTERFACE_H
#define COMMINTERFACE_H

#include "LiveData.h"
//#include "BoardInterface.h"

class CommInterface {
  
  protected:
    LiveData* liveData;   
    //BoardInterface* board;   
  public:
    void initComm(LiveData* pLiveData/*, BoardInterface* pBoard**/);
    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual void scanDevices() = 0;
};

#endif // COMMINTERFACE_H
