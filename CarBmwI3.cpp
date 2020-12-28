#include "CarBmwI3.h"
#include <vector>
#include <algorithm>

/**
   activateliveData->commandQueue
*/
void CarBmwI3::activateCommandQueue() {
	const uint16_t commandQueueLoopFrom = 18;
	
//	const std::vector<String> commandQueue = {
   const std::vector<LiveData::Command_t> commandQueue = {
		{0, "ATZ"},		// Reset all
		{0, "ATD"},		// All to defaults
		{0, "ATI"},		// Print the version ID
		{0, "ATE0"},	// Echo off
		{0, "ATPP2COFF"}, // Disable prog parameter 2C
		//{0, "ATSH6F1"},	// Set header to 6F1
		{0, "ATCF600"}, //	Set the ID filter to 600
		{0, "ATCM700"}, //	Set the ID mask to 700
		{0, "ATPBC001"}, //	 Protocol B options and baudrate (div 1 = 500k)
		{0, "ATSPB"}, //	Set protocol to B and save it (USER1 11bit, 125kbaud)
		{0, "ATAT0"}, //	Adaptive timing off
		{0, "ATSTFF"}, //	Set timeout to ff x 4ms
		{0, "ATAL"}, //	Allow long messages ( > 7 Bytes)
		{0, "ATH1"}, //	Additional headers on
		{0, "ATS0"}, //	Printing of spaces off
		{0, "ATL0"}, //	Linefeeds off
		{0, "ATCSM0"}, //	Silent monitoring off
		{0, "ATCTM5"}, //	Set timer multiplier to 5
		{0, "ATJE"}, //	Use J1939 SAE data format

		// Loop from (BMW i3)
		// BMS
		{0, "ATSH6F1"},

    {0x12, "22402B"}, // STATUS_MESSWERTE_IBS - 12V Bat
    //////{0x12, "22F101"}, // STATUS_A_T_ELUE ???
    {0x78, "22D85C"}, // Calculated indoor temperature
    {0x78, "22D96B"}, // Outdoor temperature
    //{0, "22DC61"}, // BREMSLICHT_SCHALTER
    {0x07, "22DD7B"}, // ALTERUNG_KAPAZITAET Aging of kapacity
    {0x07, "22DD7C"}, // GW_INFO - should contain kWh but in some strange form
    {0x07, "22DDBF"}, // Min and Max cell voltage
    {0x07, "22DDC0"}, // TEMPERATUREN
    {0x07, "22DD69"}, // HV_STORM
    {0x07, "22DD6C"}, // KUEHLKREISLAUF_TEMP
    {0x07, "22DDB4"}, // HV_SPANNUNG
    {0x07, "22DDBC"}  // SOC
		
		
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
  liveData->commandQueue.clear(); // probably not needed before assign
  liveData->commandQueue.assign(commandQueue.begin(), commandQueue.end());

	liveData->commandQueueLoopFrom = commandQueueLoopFrom;
	liveData->commandQueueCount = commandQueue.size();
  liveData->bAdditionalStartingChar = true; // there is one additional byte in received packets compared to other cars
  liveData->expectedMinimalPacketLength = 6;  // to filter occasional 5-bytes long packets
  liveData->rxTimeoutMs = 500; // timeout for receiving of CAN response
  liveData->delayBetweenCommandsMs = 0;//100; // delay between commands, set to 0 if no delay is needed 
}

/**
   parseRowMerged
*/
void CarBmwI3::parseRowMerged() 
{
  syslog->print("--mergedVectorLength: ");   syslog->println(liveData->vResponseRowMerged.size());

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
  
  //syslog->print("--extracted PID: "); syslog->println(pHeader->getPid());
  //syslog->print("--payload length: "); syslog->println(payloadLength);
  
  
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

        syslog->print("----batMinC: "); syslog->println(liveData->params.batMinC);
        syslog->print("----batTemp: "); syslog->println(liveData->params.batTempC);
        syslog->print("----batMaxC: "); syslog->println(liveData->params.batMaxC);
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
