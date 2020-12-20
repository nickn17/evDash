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
		"22DDBC" /*,	// SOC
		
		
		// VMCU
		"ATSH7E2",
		"2101",
		      // speed, ...
		"2102",
		      // aux, ...

		//"ATSH7Df",
		//"2106",
		//"220106",

		// Aircondition
		// IONIQ OK
		"ATSH7B3",
		"220100",
		    // in/out temp
		"220102", 
		   // coolant temp1, 2

		// BCM / TPMS
		// IONIQ OK
		"ATSH7A0",
		"22c00b",
		    // tire pressure/temp

		// CLUSTER MODULE
		// IONIQ OK
		"ATSH7C6",
		"22B002",
		    // odo
		*/
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
}

/**
   parseRowMerged
*/
void CarBmwI3::parseRowMerged() 
{


}

/**
   loadTestData
*/
void CarBmwI3::loadTestData() 
{

	
}

