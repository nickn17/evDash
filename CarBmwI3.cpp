#include "CarBmwI3.h"
#include <vector>

/**
   activateliveData->commandQueue
*/
void CarBmwI3::activateCommandQueue() {
	const uint16_t commandQueueLoopFrom = 8;
	
	const std::vector<String> commandQueue = {
		"AT Z",		// Reset all
		"AT D",		// All to defaults
		"AT I",		// Print the version ID
		"AT E0",	// Echo off
		"AT PP2COFF", // Disable prog parameter 2C
		//"AT SH6F1",	// Set header to 6F1
		"AT CF600", //	Set the ID filter to 600
		"AT CM700", //	Set the ID mask to 700
		"AT PBC001", //	 Protocol B options and baudrate (div 1 = 500k)
		"AT SPB", //	Set protocol to B and save it (USER1 11bit, 125kbaud)
		"AT AT0", //	Adaptive timing off
		"AT STFF", //	Set timeout to ff x 4ms
		"AT AL", //	Allow long messages ( > 7 Bytes)
		"AT H1", //	Additional headers on
		"AT S0", //	Printing of spaces off
		"AT L0", //	Linefeeds off
		"AT CSM0", //	Silent monitoring off
		"AT CTM5", //	Set timer multiplier to 5
		"AT JE", //	Use J1939 SAE data format

		// Loop from (BMW i3)
		// BMS
		"ATSH6F1",

    "22DDC0", // TEMPERATUREN
    "22DD69", // HV_STORM
    "22DD6C", // KUEHLKREISLAUF_TEMP
    "22DDB4", // HV_SPANNUNG
    "22DDBC"  // SOC
		
		
	};

	// 28kWh version
	liveData->params.batteryTotalAvailableKWh = 18.8;
	liveData->params.batModuleTempCount = 5; //?

	//  Empty and fill command queue
	for(auto item : liveData->commandQueue) {
		item = "";
	}

	for (int i = 0; i < commandQueue.size(); i++) {
		liveData->commandQueue[i] = commandQueue[i];
	}

	liveData->commandQueueLoopFrom = commandQueueLoopFrom;
	liveData->commandQueueCount = commandQueue.size();
  liveData->rxBuffOffset = 1; // there is one additional byte in received packets compared to other cars
}

/**
   parseRowMerged
*/
void CarBmwI3::parseRowMerged() 
{
  Serial.println("--Parsing row merged: ");  
  Serial.print("--responseRowMerged: ");  Serial.println(liveData->responseRowMerged);
  Serial.print("--currentAtshRequest: ");   Serial.println(liveData->currentAtshRequest);
  Serial.print("--commandRequest: ");       Serial.println(liveData->commandRequest);
  Serial.print("--mergedLength: ");         Serial.println(liveData->responseRowMerged.length());
  Serial.print("--mergedVectorLength: ");   Serial.println(liveData->vResponseRowMerged.size());
  if (liveData->responseRowMerged.length() <= 6) {
    Serial.println("--too short data, skiping processing");
  }

  struct Header_t
  {
    uint8_t startChar;
    uint8_t pid[2];
    uint8_t pData[];

    uint16_t getPid() { return 256 * pid[0] + pid[1]; };
  };

  Header_t* pHeader = (Header_t*)liveData->vResponseRowMerged.data();
  uint8_t* pPayload = pHeader->pData;
  const uint16_t payloadLength = liveData->vResponseRowMerged.size() - sizeof(Header_t);
  Serial.print("--extracted PID: "); Serial.println(pHeader->getPid());
  Serial.print("--payload length: "); Serial.println(payloadLength);
  
  
  // BMS
  if (liveData->currentAtshRequest.equals("ATSH6F1")) {

    switch (pHeader->getPid()) {
    case 0xDD69:
    {
      struct DD69_t {
        uint8_t batAmp[4];
        uint8_t unknown[4];
        int32_t getBatAmpRaw() { return 0x1000000 * batAmp[0] + 0x10000 * batAmp[1] + 0x100 * batAmp[2] + batAmp[3]; }
      };

      if (payloadLength == sizeof(DD69_t)) { 
        DD69_t* ptr = (DD69_t*)pHeader->pData;
      
        liveData->params.batPowerAmp =  ptr->getBatAmpRaw() / 100.0; //liveData->hexToDecFromResponse(6, 14, 4, true) / 100.0;
      
        liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
        if (liveData->params.batPowerKw < 0) // Reset charging start time
          liveData->params.chargingStartTime = liveData->params.currentTime;
      }
    }
    break;

    case 0xDD6C:
    {
      
    }
    break;

    case 0xDDB4:
    {
      struct DDB4_t {
        uint8_t batVoltage[2];
        uint16_t getBatVoltage() { return 0x100 * batVoltage[0] + batVoltage[1]; };
      };
      
      if (payloadLength == sizeof(DDB4_t)) { // HV_SPANNUNG_BATTERIE
        DDB4_t* ptr = (DDB4_t*)pHeader->pData;
        
        liveData->params.batVoltage = ptr->getBatVoltage() / 100.0;
        liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
        if (liveData->params.batPowerKw < 0) // Reset charging start time
          liveData->params.chargingStartTime = liveData->params.currentTime;
      }
    }
    break;

    case 0xDDC0:
    {
      struct DDC0_t {
        uint8_t tempMin[2];
        uint8_t tempMax[2];
        uint8_t tempAvg[2];
        uint8_t unknown[2];
        int16_t getTempMin() { return 0x100 * tempMin[0] + tempMin[1]; };
        int16_t getTempMax() { return 0x100 * tempMax[0] + tempMax[1]; };
        int16_t getTempAvg() { return 0x100 * tempAvg[0] + tempAvg[1]; };
      };
      
      if (payloadLength == sizeof(DDC0_t)) {
        DDC0_t* ptr = (DDC0_t*)pHeader->pData;
        
        liveData->params.batMinC = ptr->getTempMin() / 100.0;
        liveData->params.batTempC = ptr->getTempAvg() / 100.0;
        liveData->params.batMaxC = ptr->getTempMax() / 100.0;

        //Serial.print("----batMinC: "); Serial.println(liveData->params.batMinC);
        //Serial.print("----batTemp: "); Serial.println(liveData->params.batTempC);
        //Serial.print("----batMaxC: "); Serial.println(liveData->params.batMaxC);
      }
    }
    break;

    case 0xDDBC:
    {
      struct DDBC_t {
        uint8_t soc[2];
        uint8_t socMax[2];
        uint8_t socMin[2];
        uint8_t unknown[2];
        uint16_t getSoc() { return 0x100 * soc[0] + soc[1]; };
      };
      
      if (payloadLength == sizeof(DDBC_t)) {
        DDBC_t* ptr = (DDBC_t*)pHeader->pData;
        
        liveData->params.socPerc = ptr->getSoc() / 10.0;
      }
    }
    break;

    
    } // switch
    
  } // ATSH6F1

}

/**
   loadTestData
*/
void CarBmwI3::loadTestData() 
{

	
}
