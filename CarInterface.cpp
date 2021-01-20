#include "CarInterface.h"
#include "LiveData.h"

void CarInterface::setLiveData(LiveData* pLiveData) {
  liveData = pLiveData;
}

void CarInterface::setCommInterface(CommInterface* pCommInterface) {
  commInterface = pCommInterface;
}


void CarInterface::activateCommandQueue() {

}

void CarInterface::parseRowMerged() {
  
}

void CarInterface::loadTestData() {
  
}

void CarInterface::testHandler(String command) {
  
}
