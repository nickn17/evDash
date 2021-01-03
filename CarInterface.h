#ifndef CARINTERFACE_H
#define CARINTERFACE_H

#include "LiveData.h"

class CarInterface {
  
  protected:
  public:
    LiveData* liveData;   
    void setLiveData(LiveData* pLiveData); 
    virtual void activateCommandQueue();
    virtual void sleepCommandQueue();
    virtual bool commandAllowed() { return true; }
    virtual void parseRowMerged();
    virtual void loadTestData();  
};

#endif // CARINTERFACE_H
