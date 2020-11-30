#ifndef CARKIADEBUGOBD2_H
#define CARKIADEBUGOBD2_H

#include "CarInterface.h"

class CarKiaDebugObd2 : public CarInterface {
  
  private:
    
  public:
    void activateCommandQueue() override;
    void parseRowMerged() override;
    void loadTestData() override;  
};

#endif // CARKIADEBUGOBD2_H
