#ifndef CARRENAULTZOE_H
#define CARRENAULTZOE_H

#include "CarInterface.h"

class CarRenaultZoe : public CarInterface {
  
  protected:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;  
};

#endif // CARRENAULTZOE_H
