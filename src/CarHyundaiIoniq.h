#pragma once

#include "CarInterface.h"

class CarHyundaiIoniq : public CarInterface {
  
  protected:
    time_t lastAllowTpms;
    time_t lastAllowOdo;
    time_t lastAllowAircon;
    time_t lastAllowDriveMode;
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    bool commandAllowed() override;
    void loadTestData() override;   
};
