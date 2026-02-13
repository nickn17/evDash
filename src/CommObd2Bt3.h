#pragma once

#include "LiveData.h"
#include "CommInterface.h"

class CommObd2Bt3 : public CommInterface
{
protected:
  uint32_t lastConnectRetryMs = 0;
  bool btStackReady = false;
  bool connectAttemptPending = false;

public:
  void connectDevice() override;
  void disconnectDevice() override;
  void scanDevices() override;
  void mainLoop() override;
  void executeCommand(String cmd) override;
  void suspendDevice() override;
  void resumeDevice() override;
};
