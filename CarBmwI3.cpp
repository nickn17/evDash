#include "CarBmwI3.h"
#include <vector>
#include <algorithm>

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
  liveData->expectedMinimalPacketLength = 7;  // to filter occasional 5-bytes long packets
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
  
  
  const uint16_t payloadLength = liveData->vResponseRowMerged.size() - sizeof(Header_t);
  
  // create reversed payload to get little endian order of data
  std::vector<uint8_t> payloadReversed(pHeader->pData, pHeader->pData + payloadLength);
  std::reverse(payloadReversed.begin(), payloadReversed.end());
  
  Serial.print("--extracted PID: "); Serial.println(pHeader->getPid());
  Serial.print("--payload length: "); Serial.println(payloadLength);
  
  
  // BMS
  if (liveData->currentAtshRequest.equals("ATSH6F1")) {

    switch (pHeader->getPid()) {
    case 0xDD69:
    {
      struct DD69_t {
        uint8_t unknown[4];
        uint32_t batAmp;
      };

      if (payloadLength == sizeof(DD69_t)) { 
        DD69_t* ptr = (DD69_t*)payloadReversed.data();
      
        liveData->params.batPowerAmp =  ptr->batAmp / 100.0; //liveData->hexToDecFromResponse(6, 14, 4, true) / 100.0;
      
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
        uint16_t batVoltage;
      };
      
      if (payloadLength == sizeof(DDB4_t)) { // HV_SPANNUNG_BATTERIE
        DDB4_t* ptr = (DDB4_t*)payloadReversed.data();
        
        liveData->params.batVoltage = ptr->batVoltage / 100.0;
        liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
        if (liveData->params.batPowerKw < 0) // Reset charging start time
          liveData->params.chargingStartTime = liveData->params.currentTime;
      }
    }
    break;

    case 0xDDC0:
    {
      struct DDC0_t {
        uint8_t unknown[2];
        uint16_t tempAvg;
        uint16_t tempMax;
        uint16_t tempMin;
      };
      
      if (payloadLength == sizeof(DDC0_t)) {
        DDC0_t* ptr = (DDC0_t*)payloadReversed.data();
        
        liveData->params.batMinC = ptr->tempMin / 100.0;
        liveData->params.batTempC = ptr->tempAvg / 100.0;
        liveData->params.batMaxC = ptr->tempMax / 100.0;

        Serial.print("----batMinC: "); Serial.println(liveData->params.batMinC);
        Serial.print("----batTemp: "); Serial.println(liveData->params.batTempC);
        Serial.print("----batMaxC: "); Serial.println(liveData->params.batMaxC);
      }
    }
    break;

    case 0xDDBC:
    {
      struct DDBC_t {
        uint8_t unknown[2];
        uint16_t socMin;
        uint16_t socMax;
        uint16_t soc;
      };
      
      if (payloadLength == sizeof(DDBC_t)) {
        DDBC_t* ptr = (DDBC_t*)payloadReversed.data();
        
        liveData->params.socPerc = ptr->soc / 10.0;
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
