#ifndef CARKIANIROPHEV_H
#define CARKIANIROPHEV_H

#include "CarInterface.h"

class CarKiaNiroPhev: public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;   
};

#endif
