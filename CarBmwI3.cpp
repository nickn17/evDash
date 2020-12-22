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
		
    "22DD69", // HV_STORM
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
  if (liveData->responseRowMerged.length() <= 6) {
    Serial.println("--too short data, skiping processing");
  }
  
  // BMS
  if (liveData->currentAtshRequest.equals("ATSH6F1")) {
    if (liveData->commandRequest.equals("22DD69")) { 
      liveData->params.batPowerAmp = - liveData->hexToDecFromResponse(6, 14, 4, true) / 100.0;
    }
    
    if (liveData->commandRequest.equals("22DDB4")) { // HV_SPANNUNG_BATTERIE
      liveData->params.batVoltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 100.0;
    }

    if (liveData->commandRequest.equals("22DDBC")) {
      liveData->params.socPerc = liveData->hexToDecFromResponse(6, 10, 2, false) / 10.0;
    }
  }

}

/**
   loadTestData
*/
void CarBmwI3::loadTestData() 
{

	
}
