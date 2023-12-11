#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarPeugeotE208.h"
#include <vector>

#define commandQueueLoopFromPeugeotE208 14

/**
   activateCommandQueue
*/
void CarPeugeotE208::activateCommandQueue()
{

  std::vector<String> commandQueuePeugeotE208 = {
      "AT Z",    // Reset all
      "AT I",    // Print the version ID
      "AT S0",   // Printing of spaces on
      "AT E0",   // Echo off
      "AT L0",   // Linefeeds off
      "AT SP 6", // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
      //"AT AL",     // Allow Long (>7 byte) messages
      //"AT AR",     // Automatically receive
      //"AT H1",     // Headers on (debug only)
      //"AT D1",     // Display of the DLC on
      //"AT CAF0",   // Automatic formatting off
      ////"AT AT0",     // disabled adaptive timing
      "AT DP",
      "AT ST16", // timeout
      "ATCAF1",
      "ATSH79B",
      "ATFCSH79B",
      "ATFCSD300000",
      "ATFCSM1",
      "ATSH6A2",

      // Loop from (PEUGEOT)

      "ATFCSH6A2",
      "ATCRA682",
      "22D4341",
      "22D8EF1",
      "22D4021",
      "ATSH6B4",
      "ATFCSH6B4",
      "ATCRA694",
      "22D8161",
      "22D8151",
      "22D86F1",
      "22D8701",
      "22D4101",
      "22D8651",
      "22D8601",
      "22D8101",

  };

  //
  liveData->params.batModuleTempCount = 12; // 24, 12 is display limit
  liveData->params.batteryTotalAvailableKWh = 22;
  // usable 22, total 26

  //  Empty and fill command queue
  liveData->commandQueue.clear();
  for (auto cmd : commandQueuePeugeotE208)
  {
    liveData->commandQueue.push_back({0, cmd});
  }

  liveData->commandQueueLoopFrom = commandQueueLoopFromPeugeotE208;
  liveData->commandQueueCount = commandQueuePeugeotE208.size();
}

/**
   parseRowMerged
*/
void CarPeugeotE208::parseRowMerged()
{

  //  uint8_t tempByte;
  uint16_t tempWord;
  float tempFloat;

  if (liveData->responseRowMerged.equals("NO DATA"))
  {
    syslog->println("empty response NO DATA");
    return;
  }

  // LBC 79B
  if (liveData->currentAtshRequest.equals("ATSH79B"))
  {
    if (liveData->commandRequest.equals("2101"))
    {
      liveData->params.batPowerAmp = (liveData->hexToDecFromResponse(4, 8, 2, false) - 5000) / 10.0;
      syslog->print("liveData->params.batPowerAmp: ");
      syslog->println(liveData->params.batPowerAmp);
      liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
      syslog->print("liveData->params.batPowerKw: ");
      syslog->println(liveData->params.batPowerKw);
      if (liveData->params.batPowerKw < 0) // Reset charging start time
        liveData->params.chargingStartTime = liveData->params.currentTime;
      liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
      liveData->params.auxVoltage = liveData->hexToDecFromResponse(56, 60, 2, false) / 100.0;
      syslog->print("liveData->params.auxVoltage: ");
      syslog->println(liveData->params.auxVoltage);
      float tmpAuxPerc;
      if (liveData->params.ignitionOn)
      {
        tmpAuxPerc = (float)(liveData->params.auxVoltage - 12.8) * 100 / (float)(14.8 - 12.8); // min: 12.8V; max: 14.8V
      }
      else
      {
        tmpAuxPerc = (float)(liveData->params.auxVoltage - 11.6) * 100 / (float)(12.8 - 11.6); // min 11.6V; max: 12.8V
      }
      liveData->params.auxPerc = ((tmpAuxPerc > 100) ? 100 : ((tmpAuxPerc < 0) ? 0 : tmpAuxPerc));
      syslog->print("liveData->params.auxPerc: ");
      syslog->println(liveData->params.auxPerc);
      liveData->params.availableChargePower = liveData->hexToDecFromResponse(84, 88, 2, false) / 100.0;
      syslog->print("liveData->params.availableChargePower: ");
      syslog->println(liveData->params.availableChargePower);
    }
    if (liveData->commandRequest.equals("2103"))
    {
      liveData->params.socPercPrevious = liveData->params.socPerc;
      liveData->params.socPerc = liveData->hexToDecFromResponse(48, 52, 2, false) / 100.0;
      syslog->print("liveData->params.socPerc: ");
      syslog->println(liveData->params.socPerc);
      liveData->params.batCellMinV = liveData->hexToDecFromResponse(24, 28, 2, false) / 100.0;
      syslog->print("liveData->params.batCellMinV: ");
      syslog->println(liveData->params.batCellMinV);
      liveData->params.batCellMaxV = liveData->hexToDecFromResponse(28, 32, 2, false) / 100.0;
      syslog->print("liveData->params.batCellMaxV: ");
      syslog->println(liveData->params.batCellMaxV);
    }
    if (liveData->commandRequest.equals("2104"))
    {
      for (uint16_t i = 0; i < 12; i++)
      {
        liveData->params.batModuleTempC[i] = liveData->hexToDecFromResponse(8 + (i * 6), 10 + (i * 6), 1, false) - 40;
      }
      for (uint16_t i = 12; i < 24; i++)
      {
        liveData->params.batModuleTempC[i] = liveData->hexToDecFromResponse(80 + ((i - 12) * 6), 82 + ((i - 12) * 6), 1, false) - 40;
      }
      liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
      for (uint16_t i = 1; i < 24; i++)
      {
        if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
          liveData->params.batMinC = liveData->params.batModuleTempC[i];
        if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
          liveData->params.batMaxC = liveData->params.batModuleTempC[i];
      }
      liveData->params.batTempC = liveData->params.batMinC;
      syslog->print("liveData->params.batTempC: ");
      syslog->println(liveData->params.batTempC);
    }
    if (liveData->commandRequest.equals("2141"))
    {
      for (int i = 0; i < 62; i++)
      {
        liveData->params.cellVoltage[i] = liveData->hexToDecFromResponse(4 + (i * 4), 8 + (i * 4), 2, false) / 1000;
      }
    }
    if (liveData->commandRequest.equals("2142"))
    {
      for (int i = 0; i < 34; i++)
      {
        liveData->params.cellVoltage[i + 62] = liveData->hexToDecFromResponse(4 + (i * 4), 8 + (i * 4), 2, false) / 1000;
      }
      liveData->params.batVoltage = liveData->hexToDecFromResponse(144, 148, 2, false) / 100;
      syslog->print("liveData->params.batVoltage: ");
      syslog->println(liveData->params.batVoltage);
    }
    if (liveData->commandRequest.equals("2161"))
    {
      liveData->params.sohPerc = liveData->hexToDecFromResponse(18, 20, 2, false) / 2.0;
      syslog->print("liveData->params.sohPerc: ");
      syslog->println(liveData->params.sohPerc);
    }
  }

  // CLUSTER 743
  if (liveData->currentAtshRequest.equals("ATSH743"))
  {
    if (liveData->commandRequest.equals("220206"))
    {
      liveData->params.odoKm = liveData->hexToDecFromResponse(6, 14, 4, false);
      syslog->print("liveData->params.speedKmh: ");
      syslog->println(liveData->params.speedKmh);
    }
  }

  // CLUSTER ATSH7E4
  if (liveData->currentAtshRequest.equals("ATSH7E4"))
  {
    if (liveData->commandRequest.equals("222003"))
    {
      liveData->params.speedKmh = liveData->hexToDecFromResponse(6, 8, 2, false) / 100;
      syslog->print("liveData->params.speedKmh: ");
      syslog->println(liveData->params.speedKmh);
      if (liveData->params.speedKmh > 10)
        liveData->params.speedKmh += liveData->settings.speedCorrection;
      if (liveData->params.speedKmh < -99 || liveData->params.speedKmh > 200)
        liveData->params.speedKmh = 0;
    }
  }

  // CLIM 744 CLIMATE CONTROL
  if (liveData->currentAtshRequest.equals("ATSH744"))
  {
    if (liveData->commandRequest.equals("2121"))
    {
      liveData->responseRowMerged = "61217E1F568FFFFFFFF0047F008700813EE011AF0300813EE011AF";
      tempWord = (liveData->hexToDecFromResponse(6, 10, 2, false));
      tempFloat = (((tempWord >> 4) & 0x3ff) - 400) / 10;
      if (tempFloat > -40 && tempFloat < 50)
        liveData->params.indoorTemperature = tempFloat;
      syslog->print("liveData->params.indoorTemperature: ");
      syslog->println(liveData->params.indoorTemperature);
    }
    if (liveData->commandRequest.equals("2143"))
    {
      tempWord = (liveData->hexToDecFromResponse(26, 30, 2, false));
      tempFloat = ((tempWord >> 2) & 0xff) - 40;
      if (tempFloat > -40 && tempFloat < 50)
        liveData->params.outdoorTemperature = tempFloat;
      syslog->print("liveData->params.outdoorTemperature: ");
      syslog->println(liveData->params.outdoorTemperature);
      // liveData->params.coolantTemp1C = (liveData->hexToDecFromResponse(14, 16, 1, false) / 2) - 40;
      // liveData->params.coolantTemp2C = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
    }
  }

  /*
liveData->params.forwardDriveMode = (driveMode == 4);
liveData->params.reverseDriveMode = (driveMode == 2);
liveData->params.parkModeOrNeutral  = (driveMode == 1);
tempByte = liveData->hexToDecFromResponse(16, 18, 1, false);
liveData->params.ignitionOnPrevious = liveData->params.ignitionOn;
liveData->params.ignitionOn = (bitRead(tempByte, 5) == 1);
if (liveData->params.ignitionOnPrevious && !liveData->params.ignitionOn)
 liveData->params.automaticShutdownTimer = liveData->params.currentTime;
liveData->params.lightInfo = liveData->hexToDecFromResponse(18, 20, 1, false);
liveData->params.headLights = (bitRead(liveData->params.lightInfo, 5) == 1);
liveData->params.dayLights = (bitRead(liveData->params.lightInfo, 3) == 1);
liveData->params.brakeLightInfo = liveData->hexToDecFromResponse(14, 16, 1, false);
liveData->params.brakeLights = (bitRead(liveData->params.brakeLightInfo, 5) == 1);
liveData->params.auxPerc = liveData->hexToDecFromResponse(50, 52, 1, false);
liveData->params.auxCurrentAmp = - liveData->hexToDecFromResponse(46, 50, 2, true) / 1000.0;
liveData->params.cumulativeEnergyChargedKWh = liveData->decFromResponse(82, 90) / 10.0;
liveData->params.cumulativeEnergyDischargedKWh = liveData->decFromResponse(90, 98) / 10.0;
liveData->params.availableDischargePower = liveData->decFromResponse(20, 24) / 100.0;
//liveData->params.isolationResistanceKOhm = liveData->hexToDecFromResponse(118, 122, 2, true);
liveData->params.batFanStatus = liveData->hexToDecFromResponse(60, 62, 2, true);
liveData->params.batFanFeedbackHz = liveData->hexToDecFromResponse(62, 64, 2, true);
liveData->params.motorRpm = liveData->hexToDecFromResponse(112, 116, 2, false);
// This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
liveData->params.batInletC = liveData->hexToDecFromResponse(50, 52, 1, true);
liveData->params.bmsUnknownTempA = liveData->hexToDecFromResponse(30, 32, 1, true);
liveData->params.batHeaterC = liveData->hexToDecFromResponse(52, 54, 1, true);
liveData->params.bmsUnknownTempB = liveData->hexToDecFromResponse(82, 84, 1, true);
liveData->params.coolingWaterTempC = liveData->hexToDecFromResponse(14, 16, 1, false);
liveData->params.bmsUnknownTempC = liveData->hexToDecFromResponse(18, 20, 1, true);
liveData->params.bmsUnknownTempD = liveData->hexToDecFromResponse(46, 48, 1, true);
liveData->params.tireFrontLeftPressureBar = liveData->hexToDecFromResponse(14, 16, 2, false) / 72.51886900361;      *0.2 / 14.503773800722
liveData->params.tireFrontRightPressureBar = liveData->hexToDecFromResponse(22, 24, 2, false) / 72.51886900361;      *0.2 / 14.503773800722
liveData->params.tireRearRightPressureBar = liveData->hexToDecFromResponse(30, 32, 2, false) / 72.51886900361;     *0.2 / 14.503773800722
liveData->params.tireRearLeftPressureBar = liveData->hexToDecFromResponse(38, 40, 2, false) / 72.51886900361;      *0.2 / 14.503773800722
liveData->params.tireFrontLeftTempC = liveData->hexToDecFromResponse(16, 18, 2, false)  - 50;
liveData->params.tireFrontRightTempC = liveData->hexToDecFromResponse(24, 26, 2, false) - 50;
liveData->params.tireRearRightTempC = liveData->hexToDecFromResponse(32, 34, 2, false) - 50;
liveData->params.tireRearLeftTempC = liveData->hexToDecFromResponse(40, 42, 2, false) - 50;
          if (liveData->params.speedKmh < 10 && liveData->params.batPowerKw >= 1 && liveData->params.socPerc > 0 && liveData->params.socPerc <= 100) {
 if ( liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)])
   liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
 if ( liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)])
   liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
 liveData->params.chargingGraphBatMinTempC[int(liveData->params.socPerc)] = liveData->params.batMinC;
 liveData->params.chargingGraphBatMaxTempC[int(liveData->params.socPerc)] = liveData->params.batMaxC;
 liveData->params.chargingGraphHeaterTempC[int(liveData->params.socPerc)] = liveData->params.batHeaterC;
 liveData->params.chargingGraphWaterCoolantTempC[int(liveData->params.socPerc)] = liveData->params.coolingWaterTempC;
if (liveData->params.socPercPrevious - liveData->params.socPerc > 0) {
 byte index = (int(liveData->params.socPerc) == 4) ? 0 : (int)(liveData->params.socPerc / 10) + 1;
 if ((int(liveData->params.socPerc) % 10 == 9 || int(liveData->params.socPerc) == 4) && liveData->params.soc10ced[index] == -1) {
   liveData->params.soc10ced[index] = liveData->params.cumulativeEnergyDischargedKWh;
   liveData->params.soc10cec[index] = liveData->params.cumulativeEnergyChargedKWh;
   liveData->params.soc10odo[index] = liveData->params.odoKm;
   liveData->params.soc10time[index] = liveData->params.currentTime;
  */
}

/**
   loadTestData
*/
void CarPeugeotE208::loadTestData()
{

  /// LBC 79B
  liveData->currentAtshRequest = "ATSH79B";
  liveData->commandRequest = "2101";
  liveData->responseRowMerged = "6101134D138600000000000000000000000009980D610FB120D0000005420000000000000004ECB20000074B2927100000000000000000";
  parseRowMerged();
  liveData->commandRequest = "2103";
  liveData->responseRowMerged = "610301770D010D740000000001750174000000FFFF07D0050D410000030000000000";
  parseRowMerged();
  liveData->commandRequest = "2104";
  liveData->responseRowMerged = "61040B9D290B9F290B9D290B99290B9E290B98290B8A2A0B842A0BA0290B9B290B9A290B9629FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF29292A000000000000";
  parseRowMerged();
  liveData->commandRequest = "2141";
  liveData->responseRowMerged = "61410E930E950E900E930E930E950E950E930E970E940E970E970E950E940E940E930E970E920E930E8F0E900E8E0E8C0E920E970E920E940E940E930E950E950E940E970E970E950E970E940E950E950E990E940E9A0E8E0E900E990E950E900E990E980E950E940E970E970E950E940E980E970E920E920E940E950E93000000000000";
  parseRowMerged();
  liveData->commandRequest = "2142";
  liveData->responseRowMerged = "61420E920E940E970E930E920E970E940E950E950E980E920E900E8F0E8F0E920E900E920E940E9B0E980E950E940E950E930E970E980E980E950E950E930E970E950E950E978BFA8C200000";
  parseRowMerged();
  liveData->commandRequest = "2161";
  liveData->responseRowMerged = "6161000AA820C8C8C8C2C2000153B400004669FF";
  parseRowMerged();

  // CLUSTER 743
  liveData->currentAtshRequest = "ATSH743";
  liveData->commandRequest = "220206";
  liveData->responseRowMerged = "62020600015459";
  parseRowMerged();
}
