#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarHyundaiIoniq5.h"
#include "CommInterface.h"
#include <vector>

#define commandQueueLoopFromHyundaiIoniq5 8

// https://github.com/Esprit1st/Hyundai-Ioniq-5-Torque-Pro-PIDs
//
// To-do list for IONIQ5
// ---------------------
// D/R/P not detected - FIXED 2023-05-27 SPOT2000
// AC or DC charging is not detected properly
// Find a way to read power (current and voltage) during charing (when car is powered down)
// Engine RPM is found, but not implemented
// Find and implement GPS in car CAN messages
// Maximum charging and discharging power is found but not implemented.

/*

This is the CAN ID that responds to UDS tester present up to 0xFFF

0x05E2
0x0607
0x0703
0x0705
0x0706
0x0707
0x0725
0x0730
0x0731
0x0732
0x0733
0x0736
0x0744
0x0770 IGMP
0x0776
0x0777
0x0780
0x0781
0x0783
0x0797
0x07A0 BCM/TPM
0x07A1
0x07A3
0x07A6
0x07A7
0x07B1
0x07B6
0x07B7
0x07C4 MFC (multi function camera)
0x07C6 Cluster module
0x07C7
0x07D0
0x07D1 ABS/ESP/AHB
0x07D2
0x07D4 EPS (electrical power steering)
0x07DF
0x07E0
0x07E1
0x07E2 VMCU
0x07E3 MCU
0x07E4 BMS UDS SID 0x22 UDS DID 0x0102	0x0103	0x0104	0x0106	0x0107	0x0109	0x010B	0x010C	0x0112	0x0131	0x0132	0x0133	0x0135	0x01F4	0xF181	0xF187	0xF18B	0xF18C	0xF18E	0xF190	0xF191	0xF198	0xF199	0xF19A	0xF19B	0xF19C	0xF1A0	0xF1A1	0xF1B0	0xF1B1
0x07E5 Onboard charger
0x07E6
0x07E7
0x07E8
0x07E9
0x07EA
0x07EB
0x07EC
0x07ED
0x07EE
0x07EF
0x07F0
0x07F1
0x07F2
0x07F3
0x07F4
0x07F5
0x07F6
0x07F7
0x07F8
0x07F9
0x07FA
0x07FB
0x07FC
0x07FD
0x07FE
0x07FF
*/

/**
   activateCommandQueue
*/
void CarHyundaiIoniq5::activateCommandQueue()
{

  // Optimizer
  lastAllowTpms = 0;

  // Command queue
  std::vector<String> commandQueueHyundaiIoniq5 = {
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
      "AT ST16", // reduced timeout to 1, orig.16

      // Loop from here

      // IGPM
      "ATSH770",
      "22BC01", // 009 62BC01400000000001AAAAAAAA
      "22BC03", // low beam
      "22BC04", // 00B 62BC04B33F74EA0D002042AAAA
      "22BC05", // 00B 62BC05BF13200001000000AAAA
      "22BC06", // brake light
      "22BC07", // 00B 62BC070849DBC000101900AAAA

      // ABS / ESP + AHB
      "ATSH7D1",
      "3E00",   // UDS tester present to keep it alive even when ignition off. (test by spot2000)
      "220104", // gear selected (D/R/N/P)
      "22C101", // brake
      "22C102", // 01A 62C10237000000FFFFFFFFFFFF00FF05FFFFFF00FF5501FFFFFFAA
      "22C103", // 01A 62C103BE3000000DFFF0FCFE7FFF7FFFFFFFFFFF000005B50000AA

      "ATSH7A0",
      "22C00B", // tire pressure/temp

      // Aircondition 7B3 dont exist on IONIQ5 MY3, need to look for correct one - SPOT2000
      "ATSH7B3",
      //"3E00",   //UDS tester present to keep it alive even when ignition off. (test by spot2000)
      "220100", // in/out temp
      //"220102", // coolant temp1, 2

      // CLUSTER MODULE
      "ATSH7C6",
      "22B001", // 008 62B00100000000000000000000
      "22B002", // odo
      "22B003", // 008 62B00398000000010000000000

      // VMCU
      "ATSH7E2",
      "3E00", // UDS tester present to keep it alive even when ignition off. (test by spot2000)
      "22E003",
      "22E004",
      "22E005",
      "22E006",
      "22E007",
      "22E008",
      "22E009",

      // ICCU
      "ATSH7E5",
      "3E00",   // UDS tester present to keep it alive even when ignition off. (test by spot2000)
                /*      "22E001",
                      "22E002",
                      "22E003",*/
      "22E011", // aux soc, temp, current
                /*"22E021",
          "22E031",
          "22E032",
          "22E033",
          "22E034",
          "22E041",*/

      // MCU
      "ATSH7E3",
      "2102", // motor/invertor temp

      // AVN
      /*  "ATSH780",
        "3E00", // UDS tester present to keep it alive even when ignition off. (test by spot2000)
        "22F013",
        "22F102",
        "22F104",
        "22F112",
        "22F188",
        "22F18B",
        "22F18D",
        "22F194",
        "22F198",
        "22F1A0",
        "22F1A1",
        "22F1B1",
        "22F1B3",
        "22F1B5",
        "22F1B7",
        "22F1B9",
        "22F1DE",
        "22F183",
        "22FD85",*/

      // BMS
      "ATSH7E4",
      "3E00",   // UDS tester present to keep it alive even when ignition off. (test by spot2000)
      "220101", // power kw, engine rpm etc
      "220102", // cell voltages 1 - 32
      "220103", // cell voltages 33 - 64
      "220104", // cell voltages 65 - 96
      "220105", // soh, soc, availabe charge and discharge, DC or AC charging
      "220106", // cooling water temp
      "22010A", // cell voltages 97 - 128
      "22010B", // cell voltages 129 - 160
      "22010C", // cell voltages 161 - 180
  };

  // kWh model?
  liveData->params.batModuleTempCount = 16;
  liveData->params.batteryTotalAvailableKWh = 77;
  liveData->params.cellCount = 180;

  if (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_58)
    liveData->params.batteryTotalAvailableKWh = 58;
  if (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_72)
    liveData->params.batteryTotalAvailableKWh = 72.6;
  if (liveData->settings.carType == CAR_HYUNDAI_IONIQ5_77)
    liveData->params.batteryTotalAvailableKWh = 77.4;
  if (liveData->settings.carType == CAR_KIA_EV6_58)
    liveData->params.batteryTotalAvailableKWh = 58;
  if (liveData->settings.carType == CAR_KIA_EV6_77)
    liveData->params.batteryTotalAvailableKWh = 77.4;

  //  Empty and fill command queue
  liveData->commandQueue.clear();
  // for (int i = 0; i < commandQueueCountHyundaiIoniq5; i++) {
  for (auto cmd : commandQueueHyundaiIoniq5)
  {
    liveData->commandQueue.push_back({0, cmd}); // stxChar not used, keep it 0
  }

  //
  liveData->commandQueueLoopFrom = commandQueueLoopFromHyundaiIoniq5;
  liveData->commandQueueCount = commandQueueHyundaiIoniq5.size();
}

/**
   parseRowMerged
*/
void CarHyundaiIoniq5::parseRowMerged()
{

  uint8_t tempByte;
  //  float tempFloat;
  String tmpStr;

  // IGPM
  // RESPONDING WHEN CAR IS OFF
  if (liveData->currentAtshRequest.equals("ATSH770"))
  {
    if (liveData->commandRequest.equals("22BC03"))
    {
      //
      tempByte = liveData->hexToDecFromResponse(14, 16, 1, false);
      liveData->params.hoodDoorOpen = (bitRead(tempByte, 7) == 1);
      if (liveData->settings.rightHandDrive)
      {
        liveData->params.leftFrontDoorOpen = (bitRead(tempByte, 0) == 1);
        liveData->params.rightFrontDoorOpen = (bitRead(tempByte, 5) == 1);
        liveData->params.leftRearDoorOpen = (bitRead(tempByte, 2) == 1);
        liveData->params.rightRearDoorOpen = (bitRead(tempByte, 4) == 1);
      }
      else
      {
        liveData->params.leftFrontDoorOpen = (bitRead(tempByte, 5) == 1);
        liveData->params.rightFrontDoorOpen = (bitRead(tempByte, 0) == 1);
        liveData->params.leftRearDoorOpen = (bitRead(tempByte, 4) == 1);
        liveData->params.rightRearDoorOpen = (bitRead(tempByte, 2) == 1);
      }
      //
      tempByte = liveData->hexToDecFromResponse(16, 18, 1, false);
      liveData->params.ignitionOn = (bitRead(tempByte, 5) == 1);
      liveData->params.trunkDoorOpen = (bitRead(tempByte, 0) == 1);
      if (liveData->params.ignitionOn)
      {
        liveData->params.lastIgnitionOnTime = liveData->params.currentTime;
      }

      tempByte = liveData->hexToDecFromResponse(18, 20, 1, false);
      liveData->params.headLights = (bitRead(tempByte, 5) == 1);
      liveData->params.autoLights = (bitRead(tempByte, 4) == 1);
      liveData->params.dayLights = (bitRead(tempByte, 3) == 1);
    }
    if (liveData->commandRequest.equals("22BC06"))
    {
      tempByte = liveData->hexToDecFromResponse(14, 16, 1, false);
      liveData->params.brakeLights = (bitRead(tempByte, 5) == 1);
    }
  }

  // ABS / ESP + AHB 7D1
  // RESPONDING WHEN CAR IS OFF
  if (liveData->currentAtshRequest.equals("ATSH7D1"))
  {
    if (liveData->commandRequest.equals("220104"))
    {
      uint8_t driveMode = liveData->hexToDecFromResponse(22, 24, 1, false); // Decode gear selector status
      liveData->params.forwardDriveMode = (driveMode == 4);
      liveData->params.reverseDriveMode = (driveMode == 2);
      liveData->params.parkModeOrNeutral = (driveMode == 1);
    }

    if (liveData->commandRequest.equals("22C101"))
    {
      // Speed for eniro
      /*if (liveData->settings.carType != CAR_HYUNDAI_KONA_2020_64 && liveData->settings.carType != CAR_HYUNDAI_KONA_2020_39)
      {
        liveData->params.speedKmh = liveData->hexToDecFromResponse(18, 20, 2, false);
        if (liveData->params.speedKmh > 10)
          liveData->params.speedKmh += liveData->settings.speedCorrection;
      }*/
    }
  }

  // TPMS 7A0
  if (liveData->currentAtshRequest.equals("ATSH7A0"))
  {
    if (liveData->commandRequest.equals("22C00B"))
    {
      liveData->params.tireFrontLeftPressureBar = liveData->hexToDecFromResponse(14, 16, 2, false) / 72.51886900361;  // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireFrontRightPressureBar = liveData->hexToDecFromResponse(24, 26, 2, false) / 72.51886900361; // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireRearLeftPressureBar = liveData->hexToDecFromResponse(34, 36, 2, false) / 72.51886900361;   // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireRearRightPressureBar = liveData->hexToDecFromResponse(44, 46, 2, false) / 72.51886900361;  // === OK Valid *0.2 / 14.503773800722
      liveData->params.tireFrontLeftTempC = liveData->hexToDecFromResponse(16, 18, 2, false) - 50;                    // === OK Valid
      liveData->params.tireFrontRightTempC = liveData->hexToDecFromResponse(26, 28, 2, false) - 50;                   // === OK Valid
      liveData->params.tireRearLeftTempC = liveData->hexToDecFromResponse(36, 38, 2, false) - 50;                     // === OK Valid
      liveData->params.tireRearRightTempC = liveData->hexToDecFromResponse(46, 48, 2, false) - 50;                    // === OK Valid
    }
  }

  // Aircon 7B3
  if (liveData->currentAtshRequest.equals("ATSH7B3"))
  {
    if (liveData->commandRequest.equals("220100"))
    {
      liveData->params.indoorTemperature = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
      liveData->params.outdoorTemperature = (liveData->hexToDecFromResponse(18, 20, 1, false) / 2) - 40;
      // liveData->params.evaporatorTempC = (liveData->hexToDecFromResponse(20, 22, 1, false) / 2) - 40;
    }
    if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "00")
    {
      // liveData->params.coolantTemp1C = (liveData->hexToDecFromResponse(14, 16, 1, false) / 2) - 40;
      // liveData->params.coolantTemp2C = (liveData->hexToDecFromResponse(16, 18, 1, false) / 2) - 40;
    }
  }

  // Cluster module 7C6
  if (liveData->currentAtshRequest.equals("ATSH7C6"))
  {
    if (liveData->commandRequest.equals("22B002"))
    {
      liveData->params.odoKm = liveData->decFromResponse(18, 24);
    }
  }

  // VMCU 7E2
  if (liveData->currentAtshRequest.equals("ATSH7E2"))
  {
    if (liveData->commandRequest.equals("2101"))
    {
      /*if (liveData->settings.carType == CAR_HYUNDAI_KONA_2020_64 || liveData->settings.carType == CAR_HYUNDAI_KONA_2020_39)
      {
        liveData->params.speedKmh = liveData->hexToDecFromResponse(32, 36, 2, false) * 0.0155; // / 100.0 *1.609 = real to gps is 1.750
        if (liveData->params.speedKmh > 10)
          liveData->params.speedKmh += liveData->settings.speedCorrection;
        if (liveData->params.speedKmh < -99 || liveData->params.speedKmh > 200)
          liveData->params.speedKmh = 0;
      } */
    }
  }

  // MCU 7E3
  if (liveData->currentAtshRequest.equals("ATSH7E3"))
  {
    if (liveData->commandRequest.equals("2102"))
    {
      liveData->params.inverterTempC = liveData->hexToDecFromResponse(32, 34, 1, true);
      liveData->params.motorTempC = liveData->hexToDecFromResponse(34, 36, 1, true);
    }
  }

  // ICCU 7E5
  if (liveData->currentAtshRequest.equals("ATSH7E5"))
  {
    if (liveData->commandRequest.equals("22E011"))
    {
      liveData->params.auxCurrentAmp = -liveData->hexToDecFromResponse(30, 34, 2, true) / 1000.0;
      liveData->params.auxPerc = liveData->hexToDecFromResponse(46, 48, 1, false);
    }
  }

  // BMS 7e4
  if (liveData->currentAtshRequest.equals("ATSH7E4"))
  {
    if (liveData->commandRequest.equals("220101"))
    {
      liveData->params.operationTimeSec = liveData->hexToDecFromResponse(98, 106, 4, false);
      liveData->params.cumulativeEnergyChargedKWh = liveData->decFromResponse(82, 90) / 10.0;
      liveData->params.cumulativeEnergyDischargedKWh = liveData->decFromResponse(90, 98) / 10.0;
      liveData->params.availableChargePower = liveData->decFromResponse(16, 20) / 100.0;
      liveData->params.availableDischargePower = liveData->decFromResponse(20, 24) / 100.0;
      // liveData->params.isolationResistanceKOhm = liveData->hexToDecFromResponse(118, 122, 2, true);
      liveData->params.batFanStatus = liveData->hexToDecFromResponse(60, 62, 1, false);
      liveData->params.batFanFeedbackHz = liveData->hexToDecFromResponse(62, 64, 1, false);
      liveData->params.batPowerAmp = -liveData->hexToDecFromResponse(26, 30, 2, true) / 10.0;
      liveData->params.batVoltage = liveData->hexToDecFromResponse(30, 34, 2, false) / 10.0;
      liveData->params.batPowerKw = (liveData->params.batPowerAmp * liveData->params.batVoltage) / 1000.0;
      if (liveData->params.batPowerKw < 0) // Reset charging start time
        liveData->params.chargingStartTime = liveData->params.currentTime;
      liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
      if (liveData->settings.voltmeterEnabled == 0)
      {
        liveData->params.auxVoltage = liveData->hexToDecFromResponse(64, 66, 1, false) / 10.0;
      }
      liveData->params.batCellMaxV = liveData->hexToDecFromResponse(52, 54, 1, false) / 50.0;
      liveData->params.batCellMaxVNo = liveData->hexToDecFromResponse(54, 56, 1, false);
      liveData->params.batCellMinV = liveData->hexToDecFromResponse(56, 58, 1, false) / 50.0;
      liveData->params.batCellMinVNo = liveData->hexToDecFromResponse(58, 60, 1, false);
      liveData->params.batModuleTempC[0] = liveData->hexToDecFromResponse(38, 40, 1, true); // 1
      liveData->params.batModuleTempC[1] = liveData->hexToDecFromResponse(40, 42, 1, true); // 2
      liveData->params.batModuleTempC[2] = liveData->hexToDecFromResponse(42, 44, 1, true); // 3
      liveData->params.batModuleTempC[3] = liveData->hexToDecFromResponse(44, 46, 1, true); // 4
      liveData->params.batModuleTempC[4] = liveData->hexToDecFromResponse(46, 48, 1, true); // 5
      liveData->params.motorRpm = liveData->hexToDecFromResponse(112, 116, 2, false);
      // liveData->params.batTempC = liveData->hexToDecFromResponse(36, 38, 1, true);
      // liveData->params.batMaxC = liveData->hexToDecFromResponse(34, 36, 1, true);
      // liveData->params.batMinC = liveData->hexToDecFromResponse(36, 38, 1, true);

      // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
      liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
      for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++)
      {
        if (liveData->params.batModuleTempC[i] < liveData->params.batMinC)
          liveData->params.batMinC = liveData->params.batModuleTempC[i];
        if (liveData->params.batModuleTempC[i] > liveData->params.batMaxC)
          liveData->params.batMaxC = liveData->params.batModuleTempC[i];
      }
      liveData->params.batTempC = liveData->params.batMinC;

      liveData->params.batInletC = liveData->hexToDecFromResponse(50, 52, 1, true);
      if (liveData->params.speedKmh < 10 && liveData->params.batPowerKw >= 1 && liveData->params.socPerc > 0 && liveData->params.socPerc <= 100)
      {
        if (liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw < liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)])
          liveData->params.chargingGraphMinKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
        if (liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] < 0 || liveData->params.batPowerKw > liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)])
          liveData->params.chargingGraphMaxKw[int(liveData->params.socPerc)] = liveData->params.batPowerKw;
        liveData->params.chargingGraphBatMinTempC[int(liveData->params.socPerc)] = liveData->params.batMinC;
        liveData->params.chargingGraphBatMaxTempC[int(liveData->params.socPerc)] = liveData->params.batMaxC;
        liveData->params.chargingGraphHeaterTempC[int(liveData->params.socPerc)] = liveData->params.batHeaterC;
        liveData->params.chargingGraphWaterCoolantTempC[int(liveData->params.socPerc)] = liveData->params.coolingWaterTempC;
      }

      // Charging ON, AC/DC
      // 2022-05 NOT WORKING value is still 0x00, found chargingOn in 220106 pos 28/Y
      tempByte = liveData->hexToDecFromResponse(24, 26, 1, false); // bit 5 = DC; bit 6 = AC;
      liveData->params.chargerACconnected = (bitRead(tempByte, 6) == 1);
      liveData->params.chargerDCconnected = (bitRead(tempByte, 5) == 1);
      // liveData->params.chargingOn = (liveData->params.chargerACconnected || liveData->params.chargerDCconnected) &&
      //                              (bitRead(tempByte, 7) == 1); // 000_HV_Charging
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220102") && liveData->responseRowMerged.substring(12, 14) == "FF")
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220103"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[32 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220104"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[64 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("22010A"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[96 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("22010B"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[128 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
      }
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("22010C"))
    {
      for (int i = 0; i < 32; i++)
      {
        liveData->params.cellVoltage[160 + i] = liveData->hexToDecFromResponse(14 + (i * 2), 14 + (i * 2) + 2, 1, false) / 50;
      }
    }

    // BMS 7e4
    if (liveData->commandRequest.equals("220105"))
    {
      liveData->params.socPercPrevious = liveData->params.socPerc;
      liveData->params.sohPerc = liveData->hexToDecFromResponse(56, 60, 2, false) / 10.0;
      liveData->params.socPerc = liveData->hexToDecFromResponse(68, 70, 1, false) / 2.0;
      // if (liveData->params.socPercPrevious != liveData->params.socPerc) liveData->params.sdcardCanNotify = true;

      liveData->params.batModuleTempC[5] = liveData->hexToDecFromResponse(24, 26, 1, true);  // 6
      liveData->params.batModuleTempC[6] = liveData->hexToDecFromResponse(26, 28, 1, true);  // 7
      liveData->params.batModuleTempC[7] = liveData->hexToDecFromResponse(28, 30, 1, true);  // 8
      liveData->params.batModuleTempC[8] = liveData->hexToDecFromResponse(30, 32, 1, true);  // 9
      liveData->params.batModuleTempC[9] = liveData->hexToDecFromResponse(32, 34, 1, true);  // 10
      liveData->params.batModuleTempC[10] = liveData->hexToDecFromResponse(34, 36, 1, true); // 11
      liveData->params.batModuleTempC[11] = liveData->hexToDecFromResponse(36, 38, 1, true); // 12
      liveData->params.batModuleTempC[12] = liveData->hexToDecFromResponse(84, 86, 1, true); // 13
      liveData->params.batModuleTempC[13] = liveData->hexToDecFromResponse(86, 88, 1, true); // 14
      liveData->params.batModuleTempC[14] = liveData->hexToDecFromResponse(88, 90, 1, true); // 15
      liveData->params.batModuleTempC[15] = liveData->hexToDecFromResponse(90, 92, 1, true); // 16

      // Soc10ced table, record x0% CEC/CED table (ex. 90%->89%, 80%->79%)
      if (liveData->params.socPercPrevious - liveData->params.socPerc > 0)
      {
        byte index = (int(liveData->params.socPerc) == 4) ? 0 : (int)(liveData->params.socPerc / 10) + 1;
        if ((int(liveData->params.socPerc) % 10 == 9 || int(liveData->params.socPerc) == 4) && liveData->params.soc10ced[index] == -1)
        {
          liveData->params.soc10ced[index] = liveData->params.cumulativeEnergyDischargedKWh;
          liveData->params.soc10cec[index] = liveData->params.cumulativeEnergyChargedKWh;
          liveData->params.soc10odo[index] = liveData->params.odoKm;
          liveData->params.soc10time[index] = liveData->params.currentTime;
        }
      }
      liveData->params.bmsUnknownTempA = liveData->hexToDecFromResponse(30, 32, 1, true);
      liveData->params.batHeaterC = liveData->hexToDecFromResponse(52, 54, 1, true);
      liveData->params.bmsUnknownTempB = liveData->hexToDecFromResponse(82, 84, 1, true);

      // log 220105 to sdcard
      tmpStr = liveData->currentAtshRequest + '/' + liveData->commandRequest + '/' + liveData->responseRowMerged;
      tmpStr.toCharArray(liveData->params.debugData, tmpStr.length() + 1);
    }
    // BMS 7e4
    if (liveData->commandRequest.equals("220106"))
    {
      liveData->params.getValidResponse = true;
      tempByte = liveData->hexToDecFromResponse(54, 56, 1, false); // bit 0 = charging on, values 00, 21 (dc), 31 (ac/dc), 41 (dc) - seems like coldgate level
      liveData->params.chargingOn = (bitRead(tempByte, 0) == 1);
      if (liveData->params.chargingOn)
      {
        liveData->params.lastChargingOnTime = liveData->params.currentTime;
      }
      //
      liveData->params.coolingWaterTempC = liveData->hexToDecFromResponse(14, 16, 1, true);
      liveData->params.bmsUnknownTempC = liveData->hexToDecFromResponse(18, 20, 1, true);
      liveData->params.bmsUnknownTempD = liveData->hexToDecFromResponse(46, 48, 1, true);
      // Battery management mode
      tempByte = liveData->hexToDecFromResponse(34, 36, 1, false);
      switch (tempByte & 0xf)
      {
      case 1:
        liveData->params.batteryManagementMode = BAT_MAN_MODE_LOW_TEMPERATURE_RANGE_COOLING;
        break;
      case 3:
        liveData->params.batteryManagementMode = BAT_MAN_MODE_LOW_TEMPERATURE_RANGE;
        break;
      case 4:
        liveData->params.batteryManagementMode = BAT_MAN_MODE_COOLING;
        break;
      case 6:
        liveData->params.batteryManagementMode = BAT_MAN_MODE_OFF;
        break;
      case 0xE:
        liveData->params.batteryManagementMode = BAT_MAN_MODE_PTC_HEATER;
        break;
      default:
        liveData->params.batteryManagementMode = BAT_MAN_MODE_UNKNOWN;
      }

      // log 220106 to sdcard
      tmpStr = liveData->currentAtshRequest + '/' + liveData->commandRequest + '/' + liveData->responseRowMerged;
      tmpStr.toCharArray(liveData->params.debugData2, tmpStr.length() + 1);
      // syslog->println(liveData->params.debugData2);
    }
  }
}

/**
   Is command from queue allowed for execute, or continue with next
*/
bool CarHyundaiIoniq5::commandAllowed()
{

  /* syslog->print("Command allowed: ");
    syslog->print(liveData->currentAtshRequest);
    syslog->print(" ");
    syslog->println(liveData->commandRequest);*/

  // SleepMode Queue Filter
  if (liveData->params.sleepModeQueue)
  {
    if (liveData->commandQueueIndex < liveData->commandQueueLoopFrom)
    {
      return true;
    }
    if (liveData->commandRequest.equals("ATSH7E4"))
    {
      return true;
    }
    if (liveData->currentAtshRequest.equals("ATSH7E4") && liveData->commandRequest.equals("220105"))
    {
      return true;
    }

    return false;
  }

  // Disabled command optimizer (allows to log all car values to sdcard, but it's slow)
  if (liveData->settings.disableCommandOptimizer)
  {
    return true;
  }

  // TPMS (once per 30 secs.)
  if (liveData->commandRequest.equals("ATSH7A0"))
  {
    return lastAllowTpms + 30 < liveData->params.currentTime;
  }
  if (liveData->currentAtshRequest.equals("ATSH7A0") && liveData->commandRequest.equals("22C00B"))
  {
    if (lastAllowTpms + 30 < liveData->params.currentTime)
    {
      lastAllowTpms = liveData->params.currentTime;
    }
    else
    {
      return false;
    }
  }

  // BMS (only for SCREEN_CELLS)
  if (liveData->currentAtshRequest.equals("ATSH7E4"))
  {
    if (liveData->commandRequest.equals("220102") || liveData->commandRequest.equals("220103") || liveData->commandRequest.equals("220104") ||
        liveData->commandRequest.equals("22010A") || liveData->commandRequest.equals("22010B") || liveData->commandRequest.equals("22010C"))
    {
      if (liveData->params.displayScreen != SCREEN_CELLS && liveData->params.displayScreenAutoMode != SCREEN_CELLS)
        return false;
    }
  }

  // HUD speedup
  if (liveData->params.displayScreen == SCREEN_HUD)
  {
    // no cooling water temp
    if (liveData->currentAtshRequest.equals("ATSH7E4"))
    {
      if (liveData->commandRequest.equals("220106"))
      {
        return false;
      }
    }

    // no aircondition
    if (liveData->currentAtshRequest.equals("ATSH7B3"))
    {
      return false;
    }

    // no ODO
    if (liveData->currentAtshRequest.equals("ATSH7C6"))
    {
      return false;
    }

    // no BCM / TPMS
    if (liveData->currentAtshRequest.equals("ATSH7A0"))
    {
      return false;
    }

    // no AUX
    if (liveData->currentAtshRequest.equals("ATSH7E2") && liveData->commandRequest.equals("2102"))
    {
      return false;
    }
  }

  return true;
}

/**
   loadTestData
*/
void CarHyundaiIoniq5::loadTestData()
{

  // IGPM
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
  liveData->responseRowMerged = "6201007F9427C8FF8D85600A24110B14FFFF0FFF64FFFFFFFF0FFFFF1654668200FFFF01FFFFFFAAAA";
  parseRowMerged();
  // 220101
  liveData->commandRequest = "220101";
  liveData->responseRowMerged = "6201010C058000FFFFFFFF6686FFFFFFFFFFFFFF7CFF1073000000000000000000000000000000AAAA";
  parseRowMerged();
  // 220102
  liveData->commandRequest = "220102";
  liveData->responseRowMerged = "620102BBFDE000BBFF010001FF00002C0001880F00360794070000000000000000000000000000AAAA";
  parseRowMerged();

  // BMS ATSH7E4
  liveData->currentAtshRequest = "ATSH7E4";
  // 220101
  liveData->commandRequest = "220101";
  liveData->responseRowMerged = "620101EFFBE7EF9A0000000000013C1BAF1A16161A161A180039C406C48500008600000C2E00000B100000087600000776000ACC5B0002C415D500000663";
  parseRowMerged();
  // 220102
  liveData->commandRequest = "220102";
  liveData->responseRowMerged = "620102FFFFFFFFC4C4C4C4C4C5C4C4C4C4C4C4C4C4C4C4C4C5C4C4C4C4C4C4C4C4C4C4C4C5C4C4AAAA";
  parseRowMerged();
  // 220103
  liveData->commandRequest = "220103";
  liveData->responseRowMerged = "620103FFFFFFFFC5C5C4C5C5C5C5C5C5C5C5C5C5C5C5C4C4C5C5C5C5C5C5C5C5C4C4C5C5C5C5C5AAAA";
  parseRowMerged();
  // 220104
  liveData->commandRequest = "220104";
  liveData->responseRowMerged = "620104FFFFFFFFC5C5C5C4C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C5C4C5C5C5C5C5AAAA";
  parseRowMerged();
  // 22010A
  liveData->commandRequest = "22010A";
  liveData->responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();
  // 22010B
  liveData->commandRequest = "22010B";
  liveData->responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();
  // 22010C
  liveData->commandRequest = "22010C";
  liveData->responseRowMerged = "620104FFFFFFFFCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBCBAAAA";
  parseRowMerged();
  // 220105
  liveData->commandRequest = "220105";
  liveData->responseRowMerged = "620105FFFB740F012C01012C1A16191619181A58F262D4000050180003E8016737009C00000000000000161A1619AAAA";
  parseRowMerged();
  // 220106
  liveData->commandRequest = "220106";
  liveData->responseRowMerged = "62010617F811001A001A00004B4A0047000000000000000E00EA003100000000000000000000AAAAAA";
  parseRowMerged();

  // BCM / TPMS ATSH7A0
  liveData->currentAtshRequest = "ATSH7A0";
  // 22C00B
  liveData->commandRequest = "22C00B";
  liveData->responseRowMerged = "62C00BFFFF0000B93D0100B43E0100B43D0100BB3C0100AAAAAAAA";
  parseRowMerged();

  // ATSH7C6
  liveData->currentAtshRequest = "ATSH7C6";
  // 22b002
  liveData->commandRequest = "22B002";
  liveData->responseRowMerged = "62B002E0000000FFB400330B0000000000000000";
  parseRowMerged();

  liveData->params.batModuleTempC[0] = 28;
  liveData->params.batModuleTempC[1] = 29;
  liveData->params.batModuleTempC[2] = 30;
  liveData->params.batModuleTempC[3] = 31;
  liveData->params.batModuleTempC[4] = 32;
  liveData->params.batModuleTempC[5] = 33;
  liveData->params.batModuleTempC[6] = 34;
  liveData->params.batModuleTempC[7] = 35;
  liveData->params.batModuleTempC[8] = 36;
  liveData->params.batModuleTempC[9] = 37;
  liveData->params.batModuleTempC[10] = 38;
  liveData->params.batModuleTempC[11] = 39;
  liveData->params.batModuleTempC[12] = 40;
  liveData->params.batModuleTempC[13] = 41;
  liveData->params.batModuleTempC[14] = 42;
  liveData->params.batModuleTempC[15] = 43;

  // This is more accurate than min/max from BMS. It's required to detect kona/eniro cold gates (min 15C is needed > 43kW charging, min 25C is needed > 58kW charging)
  liveData->params.batMinC = liveData->params.batMaxC = liveData->params.batModuleTempC[0];
  for (uint16_t i = 1; i < liveData->params.batModuleTempCount; i++)
  {
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

  // DEMO DATA
  liveData->params.forwardDriveMode = true;
  liveData->params.brakeLights = true;
  liveData->params.headLights = true;
  liveData->settings.sdcardEnabled = 1;
  liveData->params.queueLoopCounter = 1;
  liveData->params.motorTempC = 4;
  liveData->params.inverterTempC = 3;
  liveData->params.trunkDoorOpen = true;
  liveData->params.leftFrontDoorOpen = true;
  liveData->params.rightFrontDoorOpen = true;
  liveData->params.leftRearDoorOpen = true;
  liveData->params.rightRearDoorOpen = true;
  liveData->params.hoodDoorOpen = true;
}

/**
   Test handler
*/
void CarHyundaiIoniq5::testHandler(const String &cmd)
{
  int8_t idx = cmd.indexOf("/");
  if (idx == -1)
    return;
  String key = cmd.substring(0, idx);
  String value = cmd.substring(idx + 1);

  // AIRCON SCANNER
  if (key.equals("bms"))
  {
    syslog->println("Scanning...");

    // SET TESTER PRESENT
    commInterface->sendPID(liveData->hexToDec("07E2", 2, false), "3E");
    delay(10);
    for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
    {
      if (commInterface->receivePID() != 0xff)
        break;
      delay(20);
    }
    delay(liveData->delayBetweenCommandsMs);

    // CHANGE SESSION
    commInterface->sendPID(liveData->hexToDec("07E2", 2, false), "1003");
    delay(10);
    for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
    {
      if (commInterface->receivePID() != 0xff)
      {
        // WAIT FOR POSITIVE ANSWER
        if (liveData->responseRowMerged.equals("5003"))
        {
          syslog->println("POSITIVE ANSWER");
          break;
        }
      }
      delay(20);
    }
    delay(liveData->delayBetweenCommandsMs);

    // test=bms/1
    for (uint16_t a = 188; a < 255; a++)
    {
      syslog->print("NEW CYCLE: ");
      syslog->println(a);
      for (uint16_t b = 0; b < 255; b++)
      // for (uint16_t c = 0; c < 255; c++)
      {
        String command = "2F";
        if (a < 16)
          command += "0";
        command += String(a, HEX);
        if (b < 16)
          command += "0";
        command += String(b, HEX);
        /*if (c < 16)
            command += "0";
          command += String(c, HEX);
        */
        command.toUpperCase();
        command += "00";

        // EXECUTE COMMAND
        // syslog->print(".");
        commInterface->sendPID(liveData->hexToDec("0770", 2, false), command);
        //      syslog->setDebugLevel(DEBUG_COMM);
        delay(10);
        for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
        {
          if (commInterface->receivePID() != 0xff)
          {
            if (!liveData->prevResponseRowMerged.equals("7F2F31") /*&& !liveData->prevResponseRowMerged.equals("")*/)
            {
              syslog->print("### \t");
              syslog->print(command);
              syslog->print(" \t");
              syslog->println(liveData->prevResponseRowMerged);
            }
            break;
          }
          delay(10);
        }
        delay(liveData->delayBetweenCommandsMs);
        //      syslog->setDebugLevel(liveData->settings.debugLevel);
      }
    }
  }
  // BATCH SCAN
  else if (key.equals("batch"))
  {
    // test=batch/1
    for (uint16_t i = 0x00; i <= 0xFF; i++)
    {
      String command = "";
      if (i < 16)
        command += "0";
      command += String(i, HEX);
      command.toUpperCase();

      syslog->print(command);
      syslog->print(" ");

      eNiroCarControl(liveData->hexToDec("07E2", 2, false), command);
    }
    for (uint16_t j = 0x20; j <= 0x2F; j++)
      for (uint16_t i = 0x00; i <= 0xFF; i++)
      {
        String command = "";
        if (j < 16)
          command += "0";
        command += String(j, HEX);
        if (i < 16)
          command += "0";
        command += String(i, HEX);
        command.toUpperCase();

        syslog->print(command);
        syslog->print(" ");

        eNiroCarControl(liveData->hexToDec("07E2", 2, false), command);
      }
  }
  // ECU SCAN
  else if (key.equals("ecu"))
  {
    // test=ecu/1
    for (uint16_t unit = 1904; unit < 2047; unit++)
    {
      String command = "2101"; /*
       if (i < 16)
         command += "0";
       command += String(i, HEX);
       command.toUpperCase();
       command += "01";*/

      eNiroCarControl(unit, command);
      // WAIT FOR POSITIVE ANSWER
      if (liveData->responseRowMerged.equals("7F2111"))
      {
        syslog->print(unit);
        syslog->println(" POSITIVE ANSWER");
      }
    }
  }
  // ONE COMMAND
  else
  {
    // test=07C6/2FB00103
    eNiroCarControl(liveData->hexToDec(key, 2, false), value);
  }
}

/**
 * Custom menu
 */
std::vector<String> CarHyundaiIoniq5::customMenu(int16_t menuId)
{
  if (menuId == MENU_CAR_COMMANDS)
    return {
        "vessOn=VESS 5sec.",
        "doorsUnlock=Unlock doors",
        "doorsLock=Lock doors",
        "chargeCableLockOff=Charge cable lock off",
        "chargeCableLockOn=Charge cable lock on",
        "roomLampOff=Room lamp off",
        "roomLampOn=Room lamp on",
        "luggageLampOff=Luggage lamp off",
        "luggageLampOn=Luggage lamp on",
        "mirrorsUnfold=Unfold mirrors",
        "mirrorsFold=Fold mirrors",
        "heatSteeringWheelOff=Heat steering wheel off",
        "heatSteeringWheelOn=Heat steering wheel on",
        "clusterIndicatorsOff=Cluster indicators off",
        "clusterIndicatorsOn=Cluster indicators on",
        "turnSignalLeftOff=Turn signal left off",
        "turnSignalLeftOn=Turn signal left on",
        "turnSignalRightOff=Turn signal right off",
        "turnSignalRightOn=Turn signal right on",
        "headLightLowOff=Head light low off",
        "headLightLowOn=Head light low on",
        "headLightHighOff=Head light high off",
        "headLightHighOn=Head light high on",
        "frontFogLightOff=Front fog light off",
        "frontFogLightOn=Front fog light on",
        "rearLightOff=Rear light off",
        "rearLightOn=Rear light on",
        "rearFogLightOff=Rear fog light off",
        "rearFogLightOn=Rear fog light on",
        "rearDefoggerOff=Rear deffoger off",
        "rearDefoggerOn=Rear deffoger on",
        "rearLeftBrakeLightOff=Left brake light off",
        "rearLeftBrakeLightOn=Left brake light on",
        "rearRightBrakeLightOff=Right brake light off",
        "rearRightBrakeLightOn=Right brake light on",
    };

  return {};
}

/**
 * Execute custom command
 */
void CarHyundaiIoniq5::carCommand(const String &cmd)
{
  if (cmd.equals("vessOn"))
  {
    eNiroCarControl(0x736, "2FF01103");
  }
  if (cmd.equals("doorsUnlock"))
  {
    eNiroCarControl(0x770, "2FBC1103");
  }
  if (cmd.equals("doorsLock"))
  {
    eNiroCarControl(0x770, "2FBC1003");
  }
  if (cmd.equals("chargeCableLockOff"))
  {
    eNiroCarControl(0x770, "2FBC4103");
  }
  if (cmd.equals("chargeCableLockOn"))
  {
    eNiroCarControl(0x770, "2FBC3F03");
  }
  if (cmd.equals("roomLampOff"))
  {
    eNiroCarControl(0x7A0, "2FB01900");
  }
  if (cmd.equals("roomLampOn"))
  {
    eNiroCarControl(0x7A0, "2FB01903");
  }
  if (cmd.equals("luggageLampOff"))
  {
    eNiroCarControl(0x770, "2FBC1C00");
  }
  if (cmd.equals("luggageLampOn"))
  {
    eNiroCarControl(0x770, "2FBC1C03");
  }
  if (cmd.equals("mirrorsUnfold"))
  {
    eNiroCarControl(0x7A0, "2FB05C03");
  }
  if (cmd.equals("mirrorsFold"))
  {
    eNiroCarControl(0x7A0, "2FB05B03");
  }
  if (cmd.equals("heatSteeringWheelOff"))
  {
    eNiroCarControl(0x7A0, "2FB05900"); // heat power
    eNiroCarControl(0x7A0, "2FB05A00"); // LED indicator
  }
  if (cmd.equals("heatSteeringWheelOn"))
  {
    eNiroCarControl(0x7A0, "2FB05903"); // heat power
    eNiroCarControl(0x7A0, "2FB05A03"); // LED indicator
  }
  if (cmd.equals("clusterIndicatorsOff"))
  {
    eNiroCarControl(0x7C6, "2FB00100");
  }
  if (cmd.equals("clusterIndicatorsOn"))
  {
    eNiroCarControl(0x7C6, "2FB00103");
  }
  if (cmd.equals("turnSignalLeftOff"))
  {
    eNiroCarControl(0x770, "2FBC1500");
  }
  if (cmd.equals("turnSignalLeftOn"))
  {
    eNiroCarControl(0x770, "2FBC1503");
  }
  if (cmd.equals("turnSignalRightOff"))
  {
    eNiroCarControl(0x770, "2FBC1600");
  }
  if (cmd.equals("turnSignalRightOn"))
  {
    eNiroCarControl(0x770, "2FBC1603");
  }
  if (cmd.equals("headLightLowOff"))
  {
    eNiroCarControl(0x770, "2FBC0100");
  }
  if (cmd.equals("headLightLowOn"))
  {
    eNiroCarControl(0x770, "2FBC0103");
  }
  if (cmd.equals("headLightHighOff"))
  {
    eNiroCarControl(0x770, "2FBC0200");
  }
  if (cmd.equals("headLightHighOn"))
  {
    eNiroCarControl(0x770, "2FBC0203");
  }
  if (cmd.equals("frontFogLightOff"))
  {
    eNiroCarControl(0x770, "2FBC0300");
  }
  if (cmd.equals("frontFogLightOn"))
  {
    eNiroCarControl(0x770, "2FBC0303");
  }
  if (cmd.equals("rearLightOff"))
  {
    eNiroCarControl(0x770, "2FBC0400");
  }
  if (cmd.equals("rearLightOn"))
  {
    eNiroCarControl(0x770, "2FBC0403");
  }
  if (cmd.equals("rearFogLightOff"))
  {
    eNiroCarControl(0x770, "2FBC0800");
  }
  if (cmd.equals("rearFogLightOn"))
  {
    eNiroCarControl(0x770, "2FBC0803");
  }
  if (cmd.equals("rearDefoggerOff"))
  {
    eNiroCarControl(0x770, "2FBC0C00");
  }
  if (cmd.equals("rearDefoggerOn"))
  {
    eNiroCarControl(0x770, "2FBC0C03");
  }
  if (cmd.equals("rearLeftBrakeLightOff"))
  {
    eNiroCarControl(0x770, "2FBC2B00");
  }
  if (cmd.equals("rearLeftBrakeLightOn"))
  {
    eNiroCarControl(0x770, "2FBC2B03");
  }
  if (cmd.equals("rearRightBrakeLightOff"))
  {
    eNiroCarControl(0x770, "2FBC2C00");
  }
  if (cmd.equals("rearRightBrakeLightOn"))
  {
    eNiroCarControl(0x770, "2FBC2C03");
  }
}

/**
 * Eniro cmds
 */
void CarHyundaiIoniq5::eNiroCarControl(const uint16_t pid, const String &cmd)
{
  // syslog->println("EXECUTING COMMAND");
  // syslog->println(cmd);
  commInterface->sendPID(pid, "3E"); // SET TESTER PRESENT
  delay(10);
  for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
  {
    if (commInterface->receivePID() != 0xff)
      break;
    delay(20);
  }
  delay(liveData->delayBetweenCommandsMs);

  commInterface->sendPID(pid, "1003"); // CHANGE SESSION
  delay(10);
  for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
  {
    if (commInterface->receivePID() != 0xff)
    {
      // WAIT FOR POSITIVE ANSWER
      if (liveData->responseRowMerged.equals("5003"))
      {
        break;
      }
    }
    delay(20);
  }
  delay(liveData->delayBetweenCommandsMs);

  // EXECUTE COMMAND
  commInterface->sendPID(pid, cmd);
  syslog->setDebugLevel(DEBUG_COMM);
  delay(10);
  for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
  {
    if (commInterface->receivePID() != 0xff)
      break;
    delay(20);
  }
  delay(liveData->delayBetweenCommandsMs);

  syslog->setDebugLevel(liveData->settings.debugLevel);
}
