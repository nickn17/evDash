#pragma once

#include "CarInterface.h"

class CarBmwI3 : public CarInterface {
  
protected:
    
public:
	void activateCommandQueue() override;
	void parseRowMerged() override;
	void loadTestData() override;  

private:
  uint8_t totalDistanceKmOffset = 0;
};
