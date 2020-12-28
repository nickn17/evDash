#include "CarBmwI3.h"
#include <vector>
#include <algorithm>

/**
   activateliveData->commandQueue
*/
void CarBmwI3::activateCommandQueue() {
	const uint16_t commandQueueLoopFrom = 18;
	
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

    "22402B", // STATUS_MESSWERTE_IBS - 12V Bat
    //////"22F101", // STATUS_A_T_ELUE ???
    "22D85C", // Calculated indoor temperature
    "22D96B", // Outdoor temperature
    //"22DC61", // BREMSLICHT_SCHALTER
    "22DD7B", // ALTERUNG_KAPAZITAET Aging of kapacity
    "22DD7C", // GW_INFO - should contain kWh but in some strange form
    "22DDBF", // Min and Max cell voltage
    "22DDC0", // TEMPERATUREN
    "22DD69", // HV_STORM
    //"22DD6C", // KUEHLKREISLAUF_TEMP
    "22DDB4", // HV_SPANNUNG
    "22DDBC"  // SOC
		
		
	};

	// 60Ah / 22kWh version
	liveData->params.batteryTotalAvailableKWh = 18.8;
	liveData->params.batModuleTempCount = 5; //?
  
  // init params which are currently not filled from parsed data
  liveData->params.tireFrontLeftPressureBar = 0;
  liveData->params.tireFrontLeftTempC = 0;
  liveData->params.tireRearLeftPressureBar = 0;
  liveData->params.tireRearLeftTempC = 0;
  liveData->params.tireFrontRightPressureBar = 0;
  liveData->params.tireFrontRightTempC = 0;
  liveData->params.tireRearRightPressureBar = 0;
  liveData->params.tireRearRightTempC = 0;

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
  liveData->expectedMinimalPacketLength = 6;  // to filter occasional 5-bytes long packets
  liveData->rxTimeoutMs = 500; // timeout for receiving of CAN response
  liveData->delayBetweenCommandsMs = 100; // delay between commands, set to 0 if no delay is needed 
}

/**
   parseRowMerged
*/
void CarBmwI3::parseRowMerged() 
{
  
//  Serial.println("--Parsing row merged: ");  
//  Serial.print("--responseRowMerged: ");  Serial.println(liveData->responseRowMerged);
//  Serial.print("--currentAtshRequest: ");   Serial.println(liveData->currentAtshRequest);
//  Serial.print("--commandRequest: ");       Serial.println(liveData->commandRequest);
//  Serial.print("--mergedLength: ");         Serial.println(liveData->responseRowMerged.length());
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
  
  //Serial.print("--extracted PID: "); Serial.println(pHeader->getPid());
  //Serial.print("--payload length: "); Serial.println(payloadLength);
  
  
  // BMS
  if (liveData->currentAtshRequest.equals("ATSH6F1")) {

    switch (pHeader->getPid()) {
    case 0x402B:
    {
      struct s402B_t {
        int16_t unknown[13];
        uint16_t auxRawCurrent;
        uint16_t auxRawVoltage;
        int16_t auxTemp;
      };

      if (payloadLength == sizeof(s402B_t)) {
        s402B_t* ptr = (s402B_t*)payloadReversed.data();

        liveData->params.auxTemperature = ptr->auxTemp / 10.0;
        liveData->params.auxVoltage = ptr->auxRawVoltage / 4000.0 + 6;
        liveData->params.auxCurrentAmp = - (ptr->auxRawCurrent / 12.5 - 200);
      }
        
    }
    break;

    case 0xD85C:
    {
      struct D85C_t {
        int8_t indoorTemp;
      };

      if (payloadLength == sizeof(D85C_t)) { 
        D85C_t* ptr = (D85C_t*)payloadReversed.data();
        liveData->params.indoorTemperature = ptr->indoorTemp;
      }
    }
    break;

    case 0xD96B:
    {
      struct D96B_t {
        uint16_t outdoorTempRaw;
      };

      if (payloadLength == sizeof(D96B_t)) { 
        D96B_t* ptr = (D96B_t*)payloadReversed.data();
        liveData->params.outdoorTemperature = (ptr->outdoorTempRaw / 2.0) - 40.0;
      }
    }
    break;
    
    case 0xDD69:
    {
      struct DD69_t {
        uint8_t unknown[4];
        int32_t batAmp;
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
      struct DD6C_t {
        int16_t tempCoolant;
      };

      if (payloadLength == sizeof(DD6C_t)) {
        DD6C_t* ptr = (DD6C_t*)payloadReversed.data();

        liveData->params.coolingWaterTempC = ptr->tempCoolant / 10.0;
        liveData->params.coolantTemp1C = ptr->tempCoolant / 10.0;
        liveData->params.coolantTemp2C = ptr->tempCoolant / 10.0;
        /*  
        float coolingWaterTempC;
        float coolantTemp1C;
        float coolantTemp2C;
        */
      }
      
    }
    break;

    case 0xDD7B:
    {
      struct DD7B_t {
        uint8_t agingOfCapacity;
      };

      if (payloadLength == sizeof(DD7B_t)) {
        DD7B_t* ptr = (DD7B_t*)payloadReversed.data();

        liveData->params.sohPerc = ptr->agingOfCapacity;
      }
      
    }
    break;

    case 0xDD7C:
    {
      struct DD7C_t {
        //uint8_t unused1;
        uint32_t discharged;
        uint32_t charged;
        uint8_t unknown[];
      };

      Serial.print("DD7C received, struct sizeof is "); Serial.println(sizeof(DD7C_t));
      if (payloadLength >= sizeof(DD7C_t)) {
        
        DD7C_t* ptr = (DD7C_t*)(payloadReversed.data() + 1); // skip one charcter on beginning (TODO: fix when pragma push/pack is done)
        liveData->params.cumulativeEnergyDischargedKWh = ptr->discharged / 100000.0;
        if (liveData->params.cumulativeEnergyDischargedKWhStart == -1)
          liveData->params.cumulativeEnergyDischargedKWhStart = liveData->params.cumulativeEnergyDischargedKWh;

        liveData->params.cumulativeEnergyChargedKWh = ptr->charged / 100000.0;
        if (liveData->params.cumulativeEnergyChargedKWhStart == -1)
          liveData->params.cumulativeEnergyChargedKWhStart = liveData->params.cumulativeEnergyChargedKWh;
      }
      
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

    case 0xDDBF:
    {
      struct DDBF_t {
        uint16_t unused[2];
        uint16_t ucellMax;
        uint16_t ucellMin;
      };
      
      if (payloadLength == sizeof(DDBF_t)) { // HV_SPANNUNG_BATTERIE
        DDBF_t* ptr = (DDBF_t*)payloadReversed.data();
        
        liveData->params.batCellMaxV = ptr->ucellMax / 1000.0;
        liveData->params.batCellMinV = ptr->ucellMin / 1000.0;
      }
    }
    break;

    case 0xDDC0:
    {
      struct DDC0_t {
        uint8_t unknown[2];
        int16_t tempAvg;
        int16_t tempMax;
        int16_t tempMin;
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

        liveData->params.socPercPrevious = liveData->params.socPerc;
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
