#ifndef COMMOBD2BLE4_H
#define COMMOBD2BLE4_H

#include "LiveData.h"
#include "CommInterface.h"

class CommObd2Ble4 : public CommInterface {
  
  protected:
    uint32_t PIN = 1234;
  public:
    void connectDevice() override;
    void disconnectDevice() override;
    void scanDevices() override;
};

#endif // COMMOBD2BLE4_H
