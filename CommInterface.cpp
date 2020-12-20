#include "CommInterface.h"
#include "BoardInterface.h"
//#include "CarInterface.h"
#include "LiveData.h"

void CommInterface::initComm(LiveData* pLiveData, BoardInterface* pBoard) {
    
    liveData = pLiveData;   
    board = pBoard; 
    response = "";  
}
