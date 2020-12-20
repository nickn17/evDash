#pragma once

#include "LiveData.h"
#include "CommInterface.h"

class CommObd2Can : public CommInterface {

  protected:
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    //
   /* void startBleScan();
    bool connectToServer(BLEAddress pAddress);
    bool doNextAtCommand();
    bool parseRow();
    bool parseRowMerged(); */
};

