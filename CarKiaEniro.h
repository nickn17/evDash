#pragma once

#include "CarInterface.h"

class CarKiaEniro : public CarInterface {
  
  protected:
    time_t lastAllowTpms;
  public:
    void activateCommandQueue() override;
    void sleepCommandQueue() override;
    void parseRowMerged() override;
    bool commandAllowed() override;
    void loadTestData() override;  
};
