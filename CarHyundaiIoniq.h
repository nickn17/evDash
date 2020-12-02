#ifndef CARHYUNDAIIONIQ_H
#define CARHYUNDAIIONIQ_H

#include "CarInterface.h"

class CarHyundaiIoniq : public CarInterface {
  
  private:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;   
};

#endif
