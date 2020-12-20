#ifndef CARRENAULTZOE_CPP
#define CARRENAULTZOE_CPP

#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarRenaultZoe.h"

#define commandQueueCountRenaultZoe 83
#define commandQueueLoopFromRenaultZoe 9

/**
   activateCommandQueue
*/
void CarRenaultZoe::activateCommandQueue() {

  String commandQueueRenaultZoe[commandQueueCountRenaultZoe] = {
    "AT Z",      // Reset all
    "AT I",      // Print the version ID
    "AT S0",     // Printing of spaces on
    "AT E0",     // Echo off
    "AT L0",     // Linefeeds off
    "AT SP 6",   // Select protocol to ISO 15765-4 CAN (11 bit ID, 500 kbit/s)
    //"AT AL",     // Allow Long (>7 byte) messages
    //"AT AR",     // Automatically receive
    //"AT H1",     // Headers on (debug only)
    //"AT D1",     // Display of the DLC on
    //"AT CAF0",   // Automatic formatting off
    ////"AT AT0",     // disabled adaptive timing
    "AT DP",
    "AT ST16",    // reduced timeout to 1, orig.16

    // Loop from (RENAULT ZOE)

    // LBC Lithium battery controller
    "ATSH79B",
    "ATFCSH79B",
    "atfcsd300010",
    "atfcsm1",
    "2101", // 034 61011383138600000000000000000000000009970D620FC920D0000005420000000000000008D80500000B202927100000000000000000
    "2103", // 01D 6103018516A717240000000001850185000000FFFF07D00516E60000030000000000
    "2104", // 04D 6104099A37098D37098F3709903709AC3609BB3609A136098B37099737098A37098437099437FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF363637000000000000
    "2141", // 07e 61410F360F380F380F360F380F380F380F380F390F390F3A0F390F3A0F390F390F380F380F390F380F380F380F390F390F380F380F3A0F390F380F380F380F390F390F350F380F360F380F380F380F380F360F380F360F350F380F360F380F360F360F350F360F360F380F380F380F3A0F380F3A0F3A0F390F390F380F36000000000000
    "2142", // 04a 61420F3A0F390F390F390F380F380F390F390F390F390F360F360F380F380F360F380F360F380F390F390F3A0F390F390F3A0F3A0F3A0F380F390F390F3A0F390F390F380F38922091F20000
    "2161", // 014 6161000AA820C8C8C8C2C20001545800004696FF

    // CLUSTER Instrument panel
    "ATSH743",
    "ATFCSH743",
    "atfcsd300010",
    "atfcsm1",
    "220201", // 62020175300168
    "220202", // 62020274710123
//    "220203", // 7F2212
//    "220204", // 7F2212
//    "220205", // 7F2212
    "220206", // 62020600015459

    // BCB 793 Battery Connection Box
    /*"ATSH792",
    "ATFCSH792",
    "atfcsd300010",
    "atfcsm1",
    "223101", to     "223114", // all with negative 7F2212*/

    // CLIM 764 CLIMATE CONTROL
    "ATSH744",
    "ATFCSH744",
    "atfcsd300010",
    "atfcsm1",
//    "2180", // NO DATA
//    "2181", // NO DATA
    "2182", // 618038303139520430343239353031520602051523080201008815
//    "2125", // 6125000000000000000000000000000000000000
//    "2126", // NO DATA
//    "2128", // NO DATA

    // EVC 7ec El vehicle controler
    "ATSH7E4",
    "ATFCSH7E4",
    "atfcsd300010",
    "atfcsm1",
    "222001", // 62200136
    "222002", // 6220020B3D
    "222003", // 6220030000
    "222004", // 62200402ED
    "222005", // 6220050532
    "222006", // 622006015459

    // PEB 77e Power Electronics Bloc
    "ATSH75A",
    "ATFCSH75A",
    "atfcsd300010",
    "atfcsm1",
    "223009", // 6230093640

    // UBP 7bc Uncoupled Braking Pedal
    "ATSH79C",
    "ATFCSH79C",
    "atfcsd300010",
    "atfcsm1",
    "21F0", // 61F0303235315204303337333733325215160C0400000101008800
    "21F1", // 61F10000000000F000000000F0000000000012061400005C91F600
    "21FE", // 61FE333731325204303337333733325215160C0400010201008800
    
  };

  //
  liveData->params.batModuleTempCount = 12; // 24, 12 is display limit
  liveData->params.batteryTotalAvailableKWh = 28;

  //  Empty and fill command queue
  for (int i = 0; i < 300; i++) {
    liveData->commandQueue[i] = "";
  }
  for (int i = 0; i < commandQueueCountRenaultZoe; i++) {
    liveData->commandQueue[i] = commandQueueRenaultZoe[i];
  }

  liveData->commandQueueLoopFrom = commandQueueLoopFromRenaultZoe;
  liveData->commandQueueCount = commandQueueCountRenaultZoe;
}

/**
   parseRowMerged
*/
void CarRenaultZoe::parseRowMerged() {

  bool tempByte;

  // LBC 79B
  if (liveData->currentAtshRequest.equals("ATSH79B")) {
    if (liveData->commandRequest.equals("2101")) {
      liveData->params.batPowerAmp = liveData->hexToDecFromResponse(4, 8, 2, false) - 5000;
      liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
      liveData->params.auxVoltage = liveData->hexToDecFromResponse(56, 60, 2, false) / 100.0;
      liveData->params.availableChargePower = liveData->hexToDecFromResponse(84, 88, 2, false) / 100.0;
      liveData->params.batCellMinV = liveData->hexToDecFromResponse(24, 28, 2, false) / 100.0;
      liveData->params.batCellMaxV = liveData->hexToDecFromResponse(28, 32, 2, false) / 100.0;
    }
    if (liveData->commandRequest.equals("2103")) {
      liveData->params.socPercPrevious = liveData->params.socPerc;
      liveData->params.socPerc = liveData->hexToDecFromResponse(48, 52, 2, false) / 100.0;
    }
    if (liveData->commandRequest.equals("2104")) {
      for (uint16_t i = 0; i < 12; i++) {
        liveData->params.batModuleTempC[i] = liveData->hexToDecFromResponse(8 + ( i * 6), 10 + (i * 6), 1, false) - 40;
      }
      for (uint16_t i = 12; i < 24; i++) {
        liveData->params.batModuleTempC[i] = liveData->hexToDecFromResponse(80 + ((i - 12) * 6), 82 + ((i - 12) * 6), 1, false) - 40;
      }
    }
    if (liveData->commandRequest.equals("2141")) {
      for (int i = 0; i < 62; i++) {
        liveData->params.cellVoltage[i] = liveData->hexToDecFromResponse(4 + (i * 4), 8 + (i * 4), 2, false) / 1000;
      }
    }
    if (liveData->commandRequest.equals("2142")) {
      for (int i = 0; i < 34; i++) {
        liveData->params.cellVoltage[i + 62] = liveData->hexToDecFromResponse(4 + (i * 4), 8 + (i * 4), 2, false) / 1000;
      }
    }
    if (liveData->commandRequest.equals("2161")) {
      liveData->params.sohPerc = liveData->hexToDecFromResponse(18, 20, 2, false) / 2.0;
    }
  }


  /* niro
    // ABS / ESP + AHB 7D1
    if (liveData->currentAtshRequest.equals("ATSH7D1")) {
      if (liveData->commandRequest.equals("22C101")) {
        uint8_t driveMode = liveData->hexToDecFromResponse(22, 24, 1, false);
        liveData->params.forwardDriveMode = (driveMode == 4);
        liveData->params.reverseDriveMode = (driveMode == 2);
        liveData->params.parkModeOrNeutral  = (driveMode == 1);
      }
    }

    // IGPM
    if (liveData->currentAtshRequest.equals("ATSH770")) {
      if (liveData->commandRequest.equals("22BC03")) {
        tempByte = liveData->hexToDecFromResponse(16, 18, 1, false);
        liveData->params.ignitionOnPrevious = liveData->params.ignitionOn;
        liveData->params.ignitionOn = (bitRead(tempByte, 5) == 1);
        if (liveData->params.ignitionOnPrevious && !liveData->params.ignitionOn)
          liveData->params.automaticShutdownTimer = liveData->params.currentTime;

        liveData->params.lightInfo = liveData->hexToDecFromResponse(18, 20, 1, false);
        liveData->params.headLights = (bitRead(liveData->params.lightInfo, 5) == 1);
        liveData->params.dayLights = (bitRead(liveData->params.lightInfo, 3) == 1);
      }
      if (liveData->commandRequest.equals("22BC06")) {
        liveData->params.brakeLightInfo = liveData->hexToDecFromResponse(14, 16, 1, false);
        liveData->params.brakeLights = (bitRead(liveData->params.brakeLightInfo, 5) == 1);
      }
    }

    // VMCU 7E2
    if (liveData->currentAtshRequest.equals("ATSH7E2")) {
      if (liveData->commandRequest.equals("2101")) {
        liveData->params.speedKmh = liveData->hexToDecFromResponse(32, 36, 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
        if (liveData->params.speedKmh < -99 || liveData->params.speedKmh > 200)
          liveData->params.speedKmh = 0;
      }
      if (liveData->commandRequest.equals("2102")) {
        liveData->params.auxPerc = liveData->hexToDecFromResponse(50, 52, 1, false);
        liveData->params.auxCurrentAmp = - liveData->hexToDecFromResponse(46, 50, 2, true) / 1000.0;
      }
    }

    // Cluster module 7c6
    if (liveData->currentAtshRequest.equals("ATSH7C6")) {
      if (liveData->commandRequest.equals("22B002")) {
        liveData->params.odoKm = liveData->decFromResponse(18, 24);
      }
    }

    // Aircon 7b3
    if (liveData->currentAtshRequest.equals("ATSH7B3")) {
      if (liveData->commandRequest.equals("220100")) {
        liveData->params.indoorTemperature = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
        liveData->params.outdoorTemperature = (liveData->hexToDecFromResponse(18, 20, 1, false) / 2) - 40;
      }
      if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "00") {
        liveData->params.coolantTemp1C = (liveData->hexToDecFromResponse(14, 16, 1, false) / 2) - 40;
        liveData->params.coolantTemp2C = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
      }
    }

    // BMS 7e4
    if (liveData->currentAtshRequest.equals("ATSH7E4")) {
      if (liveData->commandRequest.equals("220101")) {
        liveData->params.cumulativeEnergyChargedKWh = liveData->decFromResponse(82, 90) / 10.0;
        if (liveData->params.cumulativeEnergyChargedKWhStart == -1)
          liveData->params.cumulativeEnergyChargedKWhStart = liveData->params.cumulativeEnergyChargedKWh;
        liveData->params.cumulativeEnergyDischargedKWh = liveData->decFromResponse(90, 98) / 10.0;
        if (liveData->params.cumulativeEnergyDischargedKWhStart == -1)
          liveData->params.cumulativeEnergyDischargedKWhStart = liveData->params.cumulativeEnergyDischargedKWh;
        liveData->params.availableDischargePower = liveData->decFromResponse(20, 24) / 100.0;
        //liveData->params.isolationResistanceKOhm = liveData->hexToDecFromResponse(118, 122, 2, true);
        liveData->params.batFanStatus = liveData->hexToDecFromResponse(60, 62, 2, true);
        liveData->params.batFanFeedbackHz = liveData->hexToDecFromResponse(62, 64, 2, true);
        liveData->params.auxVoltage = liveData->hexToDecFromResponse(64, 66, 2, true) / 10.0;
        liveData->params.batVoltage = liveData->hexToDecFromResponse(30, 34, 2, false) / 10.0;
        if (liveData->params.batPowerKw < 0) // Reset charging start time
          liveData->params.chargingStartTime = liveData->params.currentTime;
        liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
        liveData->params.batModuleTempC[0] = liveData->hexToDecFromResponse(38, 40, 1, true);
        liveData->params.batModuleTempC[1] = liveData->hexToDecFromResponse(40, 42, 1, true);
        liveData->params.batModuleTempC[2] = liveData->hexToDecFromResponse(42, 44, 1, true);
        liveData->params.batModuleTempC[3] = liveData->hexToDecFromResponse(44, 46, 1, true);
        liveData->params.motorRpm = liveData->hexToDecFromResponse(112, 116, 2, false);

        // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
        liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
        for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++) {
          if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
            liveData->params.batMinC = liveData->params.batModuleTempC[i];
          if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
            liveData->params.batMaxC = liveData->params.batModuleTempC[i];
        }
        liveData->params.batTempC = liveData->params.batMinC;

        liveData->params.batInletC = liveData->hexToDecFromResponse(50, 52, 1, true);
        if (liveData->params.speedKmh < 10 && liveData->params.batPowerKw >= 1 && liveData->params.socPerc > 0 && liveData->params.socPerc <= 100) {
          if ( liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)])
            liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
          if ( liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)])
            liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
          liveData->params.chargingGraphBatMinTempC[int(liveData->params.socPerc)] = liveData->params.batMinC;
          liveData->params.chargingGraphBatMaxTempC[int(liveData->params.socPerc)] = liveData->params.batMaxC;
          liveData->params.chargingGraphHeaterTempC[int(liveData->params.socPerc)] = liveData->params.batHeaterC;
          liveData->params.chargingGraphWaterCoolantTempC[int(liveData->params.socPerc)] = liveData->params.coolingWaterTempC;
        }
      }
      // BMS 7e4
      if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "FF") {
        for (int i = 0; i < 32; i++) {
          liveData->params.cellVoltage[i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
        }
      }
      // BMS 7e4
      if (liveData->commandRequest.equals("220103")) {
        for (int i = 0; i < 32; i++) {
          liveData->params.cellVoltage[32 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
        }
      }
      // BMS 7e4
      if (liveData->commandRequest.equals("220104")) {
        for (int i = 0; i < 32; i++) {
          liveData->params.cellVoltage[64 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
        }
      }
      // BMS 7e4
      if (liveData->commandRequest.equals("220105")) {
        liveData->params.socPercPrevious = liveData->params.socPerc;
        liveData->params.sohPerc = liveData->hexToDecFromResponse(56, 60, 2, false) / 10.0;
        liveData->params.socPerc = liveData->hexToDecFromResponse(68, 70, 1, false) / 2.0;

        // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
        if (liveData->params.socPercPrevious - liveData->params.socPerc > 0) {
          byte index = (int(liveData->params.socPerc) == 4) ? 0 : (int)(liveData->params.socPerc / 10) + 1;
          if ((int(liveData->params.socPerc) % 10 == 9 || int(liveData->params.socPerc) == 4) && liveData->params.soc10ced[index] == -1) {
            liveData->params.soc10ced[index] = liveData->params.cumulativeEnergyDischargedKWh;
            liveData->params.soc10cec[index] = liveData->params.cumulativeEnergyChargedKWh;
            liveData->params.soc10odo[index] = liveData->params.odoKm;
            liveData->params.soc10time[index] = liveData->params.currentTime;
          }
        }
        liveData->params.bmsUnknownTempA = liveData->hexToDecFromResponse(30, 32, 1, true);
        liveData->params.batHeaterC = liveData->hexToDecFromResponse(52, 54, 1, true);
        liveData->params.bmsUnknownTempB = liveData->hexToDecFromResponse(82, 84, 1, true);
        //
        for (int i = 30; i < 32; i++) { // ai/aj position
          liveData->params.cellVoltage[96 - 30 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
        }
      }
      // BMS 7e4
      if (liveData->commandRequest.equals("220106")) {
        liveData->params.coolingWaterTempC = liveData->hexToDecFromResponse(14, 16, 1, false);
        liveData->params.bmsUnknownTempC = liveData->hexToDecFromResponse(18, 20, 1, true);
        liveData->params.bmsUnknownTempD = liveData->hexToDecFromResponse(46, 48, 1, true);
      }
    }

    // TPMS 7a0
    if (liveData->currentAtshRequest.equals("ATSH7A0")) {
      if (liveData->commandRequest.equals("22c00b")) {
        liveData->params.tireFrontLeftPressureBar = liveData->hexToDecFromResponse(14, 16, 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
        liveData->params.tireFrontRightPressureBar = liveData->hexToDecFromResponse(22, 24, 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
        liveData->params.tireRearRightPressureBar = liveData->hexToDecFromResponse(30, 32, 2, false) / 72.51886900361;    // === OK Valid *0.2 / 14.503773800722
        liveData->params.tireRearLeftPressureBar = liveData->hexToDecFromResponse(38, 40, 2, false) / 72.51886900361;     // === OK Valid *0.2 / 14.503773800722
        liveData->params.tireFrontLeftTempC = liveData->hexToDecFromResponse(16, 18, 2, false)  - 50;      // === OK Valid
        liveData->params.tireFrontRightTempC = liveData->hexToDecFromResponse(24, 26, 2, false) - 50;      // === OK Valid
        liveData->params.tireRearRightTempC = liveData->hexToDecFromResponse(32, 34, 2, false) - 50;     // === OK Valid
        liveData->params.tireRearLeftTempC = liveData->hexToDecFromResponse(40, 42, 2, false) - 50;     // === OK Valid
      }
    }
  */
}

/**
   loadTestData
*/
void CarRenaultZoe::loadTestData() {

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


  /*
       niro
    /// IGPM
    liveData->currentAtshRequest = "ATSH770";
    // 22BC03
    liveData->commandRequest = "22BC03";
    liveData->responseRowMerged = "62BC03FDEE7C730A600000AAAA";
    parseRowMerged();

    // ABS / ESP + AHB ATSH7D1
    liveData->currentAtshRequest = "ATSH7D1";
    // 2101
    liveData->commandRequest = "22C101";
    liveData->responseRowMerged = "62C1015FD7E7D0FFFF00FF04D0D400000000FF7EFF0030F5010000FFFF7F6307F207FE05FF00FF3FFFFFAAAAAAAAAAAA";
    parseRowMerged();

    // VMCU ATSH7E2
    liveData->currentAtshRequest = "ATSH7E2";
    // 2101
    liveData->commandRequest = "2101";
    liveData->responseRowMerged = "6101FFF8000009285A3B0648030000B4179D763404080805000000";
    parseRowMerged();
    // 2102
    liveData->commandRequest = "2102";
    liveData->responseRowMerged = "6102F8FFFC000101000000840FBF83BD33270680953033757F59291C76000001010100000007000000";
    liveData->responseRowMerged = "6102F8FFFC000101000000931CC77F4C39040BE09BA7385D8158832175000001010100000007000000";
    parseRowMerged();

    // "ATSH7DF",
    liveData->currentAtshRequest = "ATSH7DF";
    // 2106
    liveData->commandRequest = "2106";
    liveData->responseRowMerged = "6106FFFF800000000000000200001B001C001C000600060006000E000000010000000000000000013D013D013E013E00";
    parseRowMerged();

    // AIRCON / ACU ATSH7B3
    liveData->currentAtshRequest = "ATSH7B3";
    // 220100
    liveData->commandRequest = "220100";
    liveData->responseRowMerged = "6201007E5027C8FF7F765D05B95AFFFF5AFF11FFFFFFFFFFFF6AFFFF2DF0757630FFFF00FFFF000000";
    liveData->responseRowMerged = "6201007E5027C8FF867C58121010FFFF10FF8EFFFFFFFFFFFF10FFFF0DF0617900FFFF01FFFF000000";
    parseRowMerged();

    // BMS ATSH7E4
    liveData->currentAtshRequest = "ATSH7E4";
    // 220101
    liveData->commandRequest = "220101";
    liveData->responseRowMerged = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
    liveData->responseRowMerged = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
    parseRowMerged();
    // 220102
    liveData->commandRequest = "220102";
    liveData->responseRowMerged = "620102FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
    parseRowMerged();
    // 220103
    liveData->commandRequest = "220103";
    liveData->responseRowMerged = "620103FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCACBCACACFCCCBCBCBCBCBCBCBCBAAAA";
    parseRowMerged();
    // 220104
    liveData->commandRequest = "220104";
    liveData->responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
    parseRowMerged();
    // 220105
    liveData->commandRequest = "220105";
    liveData->responseRowMerged = "620105003fff9000000000000000000F8A86012B4946500101500DAC03E800000000AC0000C7C701000F00000000AAAA";
    liveData->responseRowMerged = "620105003FFF90000000000000000014918E012927465000015013BB03E800000000BB0000CBCB01001300000000AAAA";
    parseRowMerged();
    // 220106
    liveData->commandRequest = "220106";
    liveData->responseRowMerged = "620106FFFFFFFF14001A00240000003A7C86B4B30000000928EA00";
    parseRowMerged();

    // BCM / TPMS ATSH7A0
    liveData->currentAtshRequest = "ATSH7A0";
    // 22c00b
    liveData->commandRequest = "22c00b";
    liveData->responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
    parseRowMerged();

    // ATSH7C6
    liveData->currentAtshRequest = "ATSH7C6";
    // 22b002
    liveData->commandRequest = "22b002";
    liveData->responseRowMerged = "62B002E0000000FFB400330B0000000000000000";
    parseRowMerged();

    liveData->params.batModuleTempC[0] = 28;
    liveData->params.batModuleTempC[1] = 29;
    liveData->params.batModuleTempC[2] = 28;
    liveData->params.batModuleTempC[3] = 30;

    // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
    liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
    for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++) {
      if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
        liveData->params.batMinC = liveData->params.batModuleTempC[i];
      if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
        liveData->params.batMaxC = liveData->params.batModuleTempC[i];
    }
    liveData->params.batTempC = liveData->params.batMinC;


    //
    liveData->params.soc10ced[10] = 2200;
    liveData->params.soc10cec[10] = 2500;
    liveData->params.soc10odo[10] = 13000;
    liveData->params.soc10time[10] = 13000;
    liveData->params.soc10ced[9] = liveData->params.soc10ced[10] + 6.4;
    liveData->params.soc10cec[9] = liveData->params.soc10cec[10] + 0;
    liveData->params.soc10odo[9] = liveData->params.soc10odo[10] + 30;
    liveData->params.soc10time[9] = liveData->params.soc10time[10] + 900;
    liveData->params.soc10ced[8] = liveData->params.soc10ced[9] + 6.8;
    liveData->params.soc10cec[8] = liveData->params.soc10cec[9] + 0;
    liveData->params.soc10odo[8] = liveData->params.soc10odo[9] + 30;
    liveData->params.soc10time[8] = liveData->params.soc10time[9] + 900;
    liveData->params.soc10ced[7] = liveData->params.soc10ced[8] + 7.2;
    liveData->params.soc10cec[7] = liveData->params.soc10cec[8] + 0.6;
    liveData->params.soc10odo[7] = liveData->params.soc10odo[8] + 30;
    liveData->params.soc10time[7] = liveData->params.soc10time[8] + 900;
    liveData->params.soc10ced[6] = liveData->params.soc10ced[7] + 6.7;
    liveData->params.soc10cec[6] = liveData->params.soc10cec[7] + 0;
    liveData->params.soc10odo[6] = liveData->params.soc10odo[7] + 30;
    liveData->params.soc10time[6] = liveData->params.soc10time[7] + 900;
    liveData->params.soc10ced[5] = liveData->params.soc10ced[6] + 6.7;
    liveData->params.soc10cec[5] = liveData->params.soc10cec[6] + 0;
    liveData->params.soc10odo[5] = liveData->params.soc10odo[6] + 30;
    liveData->params.soc10time[5] = liveData->params.soc10time[6] + 900;
    liveData->params.soc10ced[4] = liveData->params.soc10ced[5] + 6.4;
    liveData->params.soc10cec[4] = liveData->params.soc10cec[5] + 0.3;
    liveData->params.soc10odo[4] = liveData->params.soc10odo[5] + 30;
    liveData->params.soc10time[4] = liveData->params.soc10time[5] + 900;
    liveData->params.soc10ced[3] = liveData->params.soc10ced[4] + 6.4;
    liveData->params.soc10cec[3] = liveData->params.soc10cec[4] + 0;
    liveData->params.soc10odo[3] = liveData->params.soc10odo[4] + 30;
    liveData->params.soc10time[3] = liveData->params.soc10time[4] + 900;
    liveData->params.soc10ced[2] = liveData->params.soc10ced[3] + 5.4;
    liveData->params.soc10cec[2] = liveData->params.soc10cec[3] + 0.1;
    liveData->params.soc10odo[2] = liveData->params.soc10odo[3] + 30;
    liveData->params.soc10time[2] = liveData->params.soc10time[3] + 900;
    liveData->params.soc10ced[1] = liveData->params.soc10ced[2] + 6.2;
    liveData->params.soc10cec[1] = liveData->params.soc10cec[2] + 0.1;
    liveData->params.soc10odo[1] = liveData->params.soc10odo[2] + 30;
    liveData->params.soc10time[1] = liveData->params.soc10time[2] + 900;
    liveData->params.soc10ced[0] = liveData->params.soc10ced[1] + 2.9;
    liveData->params.soc10cec[0] = liveData->params.soc10cec[1] + 0.5;
    liveData->params.soc10odo[0] = liveData->params.soc10odo[1] + 15;
    liveData->params.soc10time[0] = liveData->params.soc10time[1] + 900;
  */
}

#endif // CARRENAULTZOE_CPP
