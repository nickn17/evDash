#pragma once

#include <BLEDevice.h>
#include "LiveData.h"
#include "CommInterface.h"

class CommObd2Wifi : public CommInterface {

  protected:
    uint32_t PIN = 1234;
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
    void mainLoop() override;
    void executeCommand(String cmd) override;
    //
    void startWifiScan();
    bool connectToServer();
    //
};
