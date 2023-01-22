/*

First start at implementing the VW ID.3 EV into evDash
Conny Nikolaisen 2021-02-14

Rev 2022-01-28 Change of code name to MEB started

test of github integration in VScode

*/

#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include "LiveData.h"
#include "CarVWID3.h"
#include "CommInterface.h"
#include <vector>

#define commandQueueLoopFromVWID3 9 // number of commands that are only used for init, ie loop from the one after this

/**
   activateCommandQueue
   Need to revise all the commands
*/
void CarVWID3::activateCommandQueue()
{

  // Optimizer
  lastAllowTpms = 0;

  // Command queue of all
  std::vector<String> commandQueueVWID3 = {
      "AT Z",    // reset to default settings to be sure what the base setting are
      "AT I",    // Print the version ID
      "AT SP7",  // Set protokoll to 7 - ISO 15765-4 CAN (29 bit ID, 500 kbaud)
      "AT BI",   // Bypass the initialization sequence to make sure it stays at protokoll 7
      "AT CAF0", // Turns off CAN automatic formating so that PCI bytes are not inserted in the messages, or expected in the response
      "AT L0",   // Linefeeds off
      "AT DP",   // xxxxxxxxx
      "AT ST16", // reduced timeout to 1, orig.16

      // Loop from (VW ID.3)

      // UDS values sorted after header (ECU)

      // XXX
      "ATSH17FC00B9", // Sets header to 17 FC 00 B9 -> 0x17FC00B9
      //"0322465B55555555", // DC-DC current
      "22465B", // DC-DC current
      //"0322465D55555555", // DC-DC voltage
      "22465D", // DC-DC voltage

      // BMS
      "ATSH17FC007B", // Sets header to 17 FC 00 7B  -> 0x17FC007B
      "22028C",       // SoC (BMS), %
      "22F40D",       // Speed, km/h
      "227448",       // Car operation mode, XX = 0 => standby, XX = 1 => driving, XX = 4 => AC charging, XX = 6 => DC charging
      "22743B",       // cirkulation pump HV battery - flow in %
      "221E33",       // HV Battery cell # with highest voltage, V
      "221E34",       // HV Battery cell # with lowest  voltage, V
      "221E3B",       // HV Battery voltage, V
      "221E3D",       // HV Battery current, A
      "221E0E",       // HV Battery max temp and temp point, °C and #
      "221E0F",       // HV Battery min temp and temp point, °C and #
      "221620",       // PTC heater battery current, A
      "22189D",       // HV Battery cooling liquid outlet, °C
      "22189D",       // HV battery cooling liquid inlet, °C
      "222A0B",       // HV Battery temp (main value), °C
      "221E32",       // Total accumulated charged and discharge
      "220500",       // HV battery serial
      "221E1B",       // Dynamic limit for charging in ampere
      "221E1C",       // Dynamic limit for discharging in ampere

      // Execute only for "Battery cell" screen  (via commandAllowed function)
      "221EAE", // HV Battery temp point 1, °C
      "221EAF", // HV Battery temp point 2, °C
      "221EB0", // HV Battery temp point 3, °C
      "221EB1", // HV Battery temp point 4, °C
      "221EB2", // HV Battery temp point 5, °C
      "221EB3", // HV Battery temp point 6, °C
      "221EB4", // HV Battery temp point 7, °C
      "221EB5", // HV Battery temp point 8, °C
      "221EB6", // HV Battery temp point 9, °C
      "221EB7", // HV Battery temp point 10, °C
      "221EB8", // HV Battery temp point 11, °C
      "221EB9", // HV Battery temp point 12, °C
      "221EBA", // HV Battery temp point 13, °C
      "221EBB", // HV Battery temp point 14, °C
      "221EBC", // HV Battery temp point 15, °C
      "221EBD", // HV Battery temp point 16, °C
      "227425", // HV Battery temp point 17, °C
      "227426", // HV Battery temp point 18, °C
      "221E40", // HV Battery cell voltage - cell 1, V
      "221E41", // HV Battery cell voltage - cell 2, V
      "221E42", // HV Battery cell voltage - cell 3, V
      "221E43", // HV Battery cell voltage - cell 4, V
      "221E44", // HV Battery cell voltage - cell 5, V
      "221E45", // HV Battery cell voltage - cell 6, V
      "221E46", // HV Battery cell voltage - cell 7, V
      "221E47", // HV Battery cell voltage - cell 8, V
      "221E48", // HV Battery cell voltage - cell 9, V
      "221E49", // HV Battery cell voltage - cell 10, V
      "221E4A", // HV Battery cell voltage - cell 11, V
      "221E4B", // HV Battery cell voltage - cell 12, V
      "221E4C", // HV Battery cell voltage - cell 13, V
      "221E4D", // HV Battery cell voltage - cell 14, V
      "221E4E", // HV Battery cell voltage - cell 15, V
      "221E4F", // HV Battery cell voltage - cell 16, V
      "221E50", // HV Battery cell voltage - cell 17, V
      "221E51", // HV Battery cell voltage - cell 18, V
      "221E52", // HV Battery cell voltage - cell 19, V
      "221E53", // HV Battery cell voltage - cell 20, V
      "221E54", // HV Battery cell voltage - cell 21, V
      "221E55", // HV Battery cell voltage - cell 22, V
      "221E56", // HV Battery cell voltage - cell 23, V
      "221E57", // HV Battery cell voltage - cell 24, V
      "221E58", // HV Battery cell voltage - cell 25, V
      "221E59", // HV Battery cell voltage - cell 26, V
      "221E5A", // HV Battery cell voltage - cell 27, V
      "221E5B", // HV Battery cell voltage - cell 28, V
      "221E5C", // HV Battery cell voltage - cell 29, V
      "221E5D", // HV Battery cell voltage - cell 30, V
      "221E5E", // HV Battery cell voltage - cell 31, V
      "221E5F", // HV Battery cell voltage - cell 32, V
      "221E60", // HV Battery cell voltage - cell 33, V
      "221E61", // HV Battery cell voltage - cell 34, V
      "221E62", // HV Battery cell voltage - cell 35, V
      "221E63", // HV Battery cell voltage - cell 36, V
      "221E64", // HV Battery cell voltage - cell 37, V
      "221E65", // HV Battery cell voltage - cell 38, V
      "221E66", // HV Battery cell voltage - cell 39, V
      "221E67", // HV Battery cell voltage - cell 40, V
      "221E68", // HV Battery cell voltage - cell 41, V
      "221E69", // HV Battery cell voltage - cell 42, V
      "221E6A", // HV Battery cell voltage - cell 43, V
      "221E6B", // HV Battery cell voltage - cell 44, V
      "221E6C", // HV Battery cell voltage - cell 45, V
      "221E6D", // HV Battery cell voltage - cell 46, V
      "221E6E", // HV Battery cell voltage - cell 47, V
      "221E6F", // HV Battery cell voltage - cell 48, V
      "221E70", // HV Battery cell voltage - cell 49, V
      "221E71", // HV Battery cell voltage - cell 50, V
      "221E72", // HV Battery cell voltage - cell 51, V
      "221E73", // HV Battery cell voltage - cell 52, V
      "221E74", // HV Battery cell voltage - cell 53, V
      "221E75", // HV Battery cell voltage - cell 54, V
      "221E76", // HV Battery cell voltage - cell 55, V
      "221E77", // HV Battery cell voltage - cell 56, V
      "221E78", // HV Battery cell voltage - cell 57, V
      "221E79", // HV Battery cell voltage - cell 58, V
      "221E7A", // HV Battery cell voltage - cell 59, V
      "221E7B", // HV Battery cell voltage - cell 60, V
      "221E7C", // HV Battery cell voltage - cell 61, V
      "221E7D", // HV Battery cell voltage - cell 62, V
      "221E7E", // HV Battery cell voltage - cell 63, V
      "221E7F", // HV Battery cell voltage - cell 64, V
      "221E80", // HV Battery cell voltage - cell 65, V
      "221E81", // HV Battery cell voltage - cell 66, V
      "221E82", // HV Battery cell voltage - cell 67, V
      "221E83", // HV Battery cell voltage - cell 68, V
      "221E84", // HV Battery cell voltage - cell 69, V
      "221E85", // HV Battery cell voltage - cell 70, V
      "221E86", // HV Battery cell voltage - cell 71, V
      "221E87", // HV Battery cell voltage - cell 72, V
      "221E88", // HV Battery cell voltage - cell 73, V
      "221E89", // HV Battery cell voltage - cell 74, V
      "221E8A", // HV Battery cell voltage - cell 75, V
      "221E8B", // HV Battery cell voltage - cell 76, V
      "221E8C", // HV Battery cell voltage - cell 77, V
      "221E8D", // HV Battery cell voltage - cell 78, V
      "221E8E", // HV Battery cell voltage - cell 79, V
      "221E8F", // HV Battery cell voltage - cell 80, V
      "221E90", // HV Battery cell voltage - cell 81, V
      "221E91", // HV Battery cell voltage - cell 82, V
      "221E92", // HV Battery cell voltage - cell 83, V
      "221E93", // HV Battery cell voltage - cell 84, V
      "221E94", // HV Battery cell voltage - cell 85, V
      "221E95", // HV Battery cell voltage - cell 86, V
      "221E96", // HV Battery cell voltage - cell 87, V
      "221E97", // HV Battery cell voltage - cell 88, V
      "221E98", // HV Battery cell voltage - cell 89, V
      "221E99", // HV Battery cell voltage - cell 90, V
      "221E9A", // HV Battery cell voltage - cell 91, V
      "221E9B", // HV Battery cell voltage - cell 92, V
      "221E9C", // HV Battery cell voltage - cell 93, V
      "221E9D", // HV Battery cell voltage - cell 94, V
      "221E9E", // HV Battery cell voltage - cell 95, V
      "221E9F", // HV Battery cell voltage - cell 96, V
      "221EA0", // HV Battery cell voltage - cell 97, V
      "221EA1", // HV Battery cell voltage - cell 98, V
      "221EA2", // HV Battery cell voltage - cell 99, V
      "221EA3", // HV Battery cell voltage - cell 100, V
      "221EA4", // HV Battery cell voltage - cell 101, V
      "221EA5", // HV Battery cell voltage - cell 102, V
      "221EA6", // HV Battery cell voltage - cell 103, V
      "221EA7", // HV Battery cell voltage - cell 104, V
      "221EA8", // HV Battery cell voltage - cell 105, V
      "221EA9", // HV Battery cell voltage - cell 106, V
      "221EAA", // HV Battery cell voltage - cell 107, V
      "221EAB", // HV Battery cell voltage - cell 108, V

      // ECU XXXX
      "ATSH17FC0076", // Sets header to 17 FC 00 76
      "220364",       // HV auxilary consumer power, kW
      "22295A",       // ODOMETER, km
      "22210E",       // Driving mode position (P-N-D-B), YY=08->P,YY=05->D,YY=0c->B,YY=07->R,YY=06->N
      "22F802",       // VIN number

      // ECU XXXX
      //"ATSH00000767", // Sets header to 00 00 07 67
      "ATSH767", // Sets header to 00 00 07 67
      "2222B3",  // GPS time
      "222430",  // GPS multiframe data lat, long, init, height, quality
      "222431",  // GPS number of tracked and visual satellites

      // ECU XXXX
      "ATSH746", // Sets header to 00 00 07 46
      "222613",  // Inside temperature, °C
      "222609",  // Outside temperature,  °C
      "22263B",  // Recirculation of air, XX=00 -> fresh air, XX=04 -> manual recirculation
      "2242DB",  // CO2 content interior, ppm
      "22F449",  // Accelerator pedal position, %
      "220801",  // PTC air heater inside, ampere
      "220800",  // A/C compressor multiframe

      // ECU XXXX
      "ATSH710", // Sets header to 00 00 07 10
      "222AB2",  // HV battery max energy content Wh
      //"222AB8",  // HV battery energy content
      "222AF7", // 12V multiframe

  };

  /*
  Battery	capacity

  Name    Net (kWh)    Gross (kWh)   defined name
  Pure    45           48            CAR_VW_ID3_2021_45
  Pro     58           62            CAR_VW_ID3_2021_58
  Pro S   77           82            CAR_VW_ID3_2021_77

  Find a way to identify the capacity and by that the number of cells to look after.

*/

  liveData->params.batModuleTempCount = 18;       // Maximum temperture points in battery, 18 for the VW ID.3 but maximum limit to show is 12 in evDash
  liveData->params.batteryTotalAvailableKWh = 58; // defines that Pro trim is the standard guess
  liveData->params.cellCount = 108;

  if (liveData->settings.carType == CAR_AUDI_Q4_35)
    liveData->params.batteryTotalAvailableKWh = 52;
  if (liveData->settings.carType == CAR_AUDI_Q4_40)
    liveData->params.batteryTotalAvailableKWh = 77;
  if (liveData->settings.carType == CAR_AUDI_Q4_45)
    liveData->params.batteryTotalAvailableKWh = 77;
  if (liveData->settings.carType == CAR_AUDI_Q4_50)
    liveData->params.batteryTotalAvailableKWh = 77;
  if (liveData->settings.carType == CAR_SKODA_ENYAQ_55)
    liveData->params.batteryTotalAvailableKWh = 45;
  if (liveData->settings.carType == CAR_SKODA_ENYAQ_62)
    liveData->params.batteryTotalAvailableKWh = 58;
  if (liveData->settings.carType == CAR_SKODA_ENYAQ_82)
    liveData->params.batteryTotalAvailableKWh = 77;
  if (liveData->settings.carType == CAR_VW_ID3_2021_45)
    liveData->params.batteryTotalAvailableKWh = 45;
  if (liveData->settings.carType == CAR_VW_ID3_2021_58)
    liveData->params.batteryTotalAvailableKWh = 58;
  if (liveData->settings.carType == CAR_VW_ID3_2021_77)
    liveData->params.batteryTotalAvailableKWh = 77;
  if (liveData->settings.carType == CAR_VW_ID4_2021_45)
    liveData->params.batteryTotalAvailableKWh = 45;
  if (liveData->settings.carType == CAR_VW_ID4_2021_58)
    liveData->params.batteryTotalAvailableKWh = 58;
  if (liveData->settings.carType == CAR_VW_ID4_2021_77)
    liveData->params.batteryTotalAvailableKWh = 77;

  //  Empty and fill command queue
  liveData->commandQueue.clear();

  for (auto cmd : commandQueueVWID3)
  {
    liveData->commandQueue.push_back({0, cmd}); // stxChar not used, keep it 0
  }

  //
  liveData->commandQueueLoopFrom = commandQueueLoopFromVWID3;
  liveData->commandQueueCount = commandQueueVWID3.size();
  if (liveData->settings.carType == CAR_VW_ID3_2021_58)
  {
    liveData->rxTimeoutMs = 500;          // timeout for receiving of CAN response
    liveData->delayBetweenCommandsMs = 0; // delay between commands, set to 0 if no delay is needed
  }
}

/**
   parseRowMerged, from here it starts parsing all the answers from different ECU
*/
void CarVWID3::parseRowMerged()
{

  uint8_t tempByte;
  //   float tempFloat;
  String tmpStr;

  // Fix for MEB (missing opTime in car)
  liveData->params.operationTimeSec = liveData->params.currentTime;

  // New parser for VW ID.3

  // ATSHFC00B9
  if (liveData->currentAtshRequest.equals("ATSH17FC00B9")) // For data after this header
  {
    if (liveData->commandRequest.equals("22465B")) // DC-DC converter HV to 12V current
    {
      // Put code here to parse the data
      // liveData->params.batPowerAmp = liveData->hexToDecFromResponse(6, 10, 2, false) / 16;
    }
    if (liveData->commandRequest.equals("22465D")) // DC-DC converter HV to 12V Voltage
    {
      // Put code here to parse the data
      // liveData->params.batVoltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 512;
    }
  }

  // ATSH17FC007B
  if (liveData->currentAtshRequest.equals("ATSH17FC007B")) // For data after this header
  {
    if (liveData->commandRequest.equals("22028C")) // SOC BMS %
    {
      // Put code here to parse the data
      liveData->params.socPercPrevious = liveData->params.socPerc;
      liveData->params.socPercBms = liveData->hexToDecFromResponse(6, 8, 1, false) / 2.5; // SOC BMS
      liveData->params.socPerc = liveData->hexToDecFromResponse(6, 8, 1, false)*0,4425 - 6,1947; // SOC HMI (New calculation - 2022-12-01)
       
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
    }
    if (liveData->commandRequest.equals("22F40D")) // speed km/h
    {
      // Put code here to parse the data
      liveData->params.speedKmh = liveData->hexToDecFromResponse(6, 8, 1, false); // speed from car in km/h (not GPS)
      if (liveData->params.speedKmh > 10)
        liveData->params.speedKmh += liveData->settings.speedCorrection;
    }

    if (liveData->commandRequest.equals("227448")) // Car operation mode, XX = 0 => standby, XX = 1 => driving, XX = 4 => AC charging, XX = 6 => DC charging
    {

      if (liveData->hexToDecFromResponse(6, 8, 1, false) == 1)
      {
        liveData->params.ignitionOn = true; // ignition on
        liveData->params.lastIgnitionOnTime = liveData->params.currentTime;
        liveData->params.chargingOn = false;         // Charging off
        liveData->params.chargerACconnected = false; // AC charger disconnected
        liveData->params.chargerDCconnected = false; // DC charger disconnected
      }
      if (liveData->hexToDecFromResponse(6, 8, 1, false) == 4)
        liveData->params.chargerACconnected = true; // AC charger connected
      if (liveData->hexToDecFromResponse(6, 8, 1, false) == 6)
        liveData->params.chargerDCconnected = true; // DC charger connected
      if (liveData->params.chargerACconnected || liveData->params.chargerDCconnected)
        liveData->params.chargingOn = true; // Charging on
      if (liveData->params.chargingOn)
      {
        liveData->params.lastChargingOnTime = liveData->params.currentTime;
      }
    }
    if (liveData->commandRequest.equals("22743B")) // cirkulation pump HV battery - flow in %
    {
      // kräver en ny variabel
      if (liveData->hexToDecFromResponse(6, 8, 1, false) > 0)
        liveData->params.batFanStatus = liveData->hexToDecFromResponse(6, 8, 1, false);
      liveData->params.batFanFeedbackHz = liveData->hexToDecFromResponse(6, 8, 1, false);
    }

    if (liveData->commandRequest.equals("221E33")) // HV Battery cell # with highest voltage, V
    {
      liveData->params.batCellMaxV = liveData->hexToDecFromResponse(6, 10, 2, false) / 4096; // Cell voltage, cell 1
    }

    if (liveData->commandRequest.equals("221E34")) // HV Battery cell # with lowest  voltage, V
    {
      liveData->params.batCellMinV = liveData->hexToDecFromResponse(6, 10, 2, false) / 4096; // Cell voltage, cell 1
    }

    if (liveData->commandRequest.equals("221E3B")) // HV Battery voltage, V
    {
      liveData->params.batVoltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 4; // kod här
    }

    if (liveData->commandRequest.equals("221E3D")) // HV Battery current, A
    {
      liveData->params.batPowerAmp = (liveData->hexToDecFromResponse(6, 14, 4, false) - 150000) / 100; // kod här
      liveData->params.batPowerKw = liveData->params.batPowerAmp * liveData->params.batVoltage / 1000; // total power in kW from HV battery
      liveData->params.batPowerKwh100 = liveData->params.batPowerKw / liveData->params.speedKmh * 100;
      if (liveData->params.batPowerKw < 0) // Reset charging start time
        liveData->params.chargingStartTime = liveData->params.currentTime;
    }

    if (liveData->commandRequest.equals("221E0E")) // HV Battery max temp and temp point, °C and #
    {
      liveData->params.batMaxC = liveData->hexToDecFromResponse(6, 10, 2, false) / 64; // kod här // kod här
    }

    if (liveData->commandRequest.equals("221E0F")) // HV Battery min temp and temp point, °C and #
    {
      liveData->params.batMinC = liveData->hexToDecFromResponse(6, 10, 2, false) / 64; // kod här
    }

    if (liveData->commandRequest.equals("221620")) // PTC heater battery current, A
    {
      // kod här
    }

    if (liveData->commandRequest.equals("22189D")) // HV battery cooling liquid inlet and outlet, °C
    {
      liveData->params.batInletC = liveData->hexToDecFromResponse(10, 14, 2, false) / 64; // kod här
      // liveData->params.batOutletC = liveData->hexToDecFromResponse(6, 10, 2, false) / 64; // kod här
    }

    if (liveData->commandRequest.equals("222A0B")) // HV Battery temp (main value), °C
    {
      liveData->params.batTempC = liveData->hexToDecFromResponse(6, 8, 1, false) / 2 - 40; // kod här
    }

    if (liveData->commandRequest.equals("221E1B")) // Dynamic limit for charging in ampere
    {
      liveData->params.availableChargePower = (liveData->hexToDecFromResponse(6, 10, 2, false) / 5) * liveData->params.batVoltage / 1000; // available charge power in kW
    }

    if (liveData->commandRequest.equals("221E1C")) // Dynamic limit for discharging in ampere
    {
      liveData->params.availableDischargePower = (liveData->hexToDecFromResponse(6, 10, 2, false) / 5) * liveData->params.batVoltage / 1000; // available discharge power in kW
    }

    // Here is the 18 temperature points in the HV battery

    if (liveData->commandRequest.equals("221EAE")) // HV Battery temp point 1, °C
    {
      liveData->params.batModuleTempC[0] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EAF")) // HV Battery temp point 2, °C
    {
      liveData->params.batModuleTempC[1] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB0")) // HV Battery temp point 3, °C
    {
      liveData->params.batModuleTempC[2] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB1")) // HV Battery temp point 4, °C
    {
      liveData->params.batModuleTempC[3] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB2")) // HV Battery temp point 5, °C
    {
      liveData->params.batModuleTempC[4] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB3")) // HV Battery temp point 6, °C
    {
      liveData->params.batModuleTempC[5] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB4")) // HV Battery temp point 7, °C
    {
      liveData->params.batModuleTempC[6] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB5")) // HV Battery temp point 8, °C
    {
      liveData->params.batModuleTempC[7] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB6")) // HV Battery temp point 9, °C
    {
      liveData->params.batModuleTempC[8] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB7")) // HV Battery temp point 10, °C
    {
      liveData->params.batModuleTempC[9] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB8")) // HV Battery temp point 11, °C
    {
      liveData->params.batModuleTempC[10] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EB9")) // HV Battery temp point 12, °C
    {
      liveData->params.batModuleTempC[11] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EBA")) // HV Battery temp point 13, °C
    {
      liveData->params.batModuleTempC[12] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EBB")) // HV Battery temp point 14, °C
    {
      liveData->params.batModuleTempC[13] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EBC")) // HV Battery temp point 15, °C
    {
      liveData->params.batModuleTempC[14] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("221EBD")) // HV Battery temp point 16, °C
    {
      liveData->params.batModuleTempC[15] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("227425")) // HV Battery temp point 17, °C
    {
      liveData->params.batModuleTempC[16] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    if (liveData->commandRequest.equals("227426")) // HV Battery temp point 18, °C
    {
      liveData->params.batModuleTempC[17] = liveData->hexToDecFromResponse(6, 10, 2, false) / 8 - 40; // HV Battery temp point #1
    }

    // Cell voltages
    if ((liveData->hexToDec(liveData->commandRequest, 3, false) >= 0x221E40 && liveData->hexToDec(liveData->commandRequest, 3, false) <= 0x221EAB) &&
        (!liveData->responseRowMerged.substring(6, 10).equals("0FFE"))) // 5.09 - unused cell
    {
      tempByte = liveData->hexToDec(liveData->commandRequest.substring(4, 6).c_str(), 1, false);
      liveData->params.cellVoltage[tempByte - 64] = liveData->hexToDecFromResponse(6, 10, 2, false) / 1000 + 1; // Cell voltage, cell 1
    }

    // HV Battery total accumulated charge and total accumulated discharge, MF answer
    if (liveData->commandRequest.equals("221E32"))
    {
      liveData->params.cumulativeEnergyChargedKWh = liveData->hexToDecFromResponse(22, 30, 4, false) / 8583.07123641215;        // beräkning av totalt accumulerat laddat
      liveData->params.cumulativeEnergyDischargedKWh = abs(liveData->hexToDecFromResponse(30, 38, 4, true) / 8583.07123641215); // beräkning av totalt accumulerat urladdat

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
    }
  }

  // ATSH17FC0076
  if (liveData->currentAtshRequest.equals("ATSH17FC0076")) // For data after this header
  {
    if (liveData->commandRequest.equals("220364")) // HV auxilary consumer power, kW
    {
      // Put code here to parse the data
    }
    if (liveData->commandRequest.equals("22295A")) // ODOMETER, km
    {
      liveData->params.odoKm = liveData->hexToDecFromResponse(6, 12, 3, false); // Total odometer in km
    }
    if (liveData->commandRequest.equals("22210E")) // Driving mode position (P-N-D-B), YY=08->P,YY=05->D,YY=12->B,YY=07->R,YY=06->N
    {
      if ((liveData->hexToDecFromResponse(8, 10, 1, false) == 5) || (liveData->hexToDecFromResponse(8, 10, 1, false) == 12))
      {
        liveData->params.forwardDriveMode = true;    // D or B mode
        liveData->params.chargerACconnected = false; // AC Kabel ur
        liveData->params.chargerDCconnected = false; // DC Kabel ur
        liveData->params.chargingOn = false;         // Charging Off
      }
      if (liveData->hexToDecFromResponse(8, 10, 1, false) == 7)
      {
        liveData->params.reverseDriveMode = true;    // R mode
        liveData->params.chargerACconnected = false; // AC Kabel ur
        liveData->params.chargerDCconnected = false; // DC Kabel ur
        liveData->params.chargingOn = false;         // Charging Off
      }
      if ((liveData->hexToDecFromResponse(8, 10, 1, false) == 6) || (liveData->hexToDecFromResponse(8, 10, 1, false) == 8))
      {
        liveData->params.parkModeOrNeutral = true; // N or P mode
      }
    }
  }

  // ATSH00000767
  if (liveData->settings.gpsHwSerialPort == 255) // GPS from car and no external GPS attached
  {
    if (liveData->currentAtshRequest.equals("ATSH767")) // For data after this header
    {
      if (liveData->commandRequest.equals("2222B3") && !liveData->params.currTimeSyncWithGps)
      {
        // 6222B30100585F1792AAAAAAAA
        //           ^^^^^^^^
        uint32_t dt = liveData->hexToDecFromResponse(10, 18, 4, false); // Time
        struct tm tm;
        tm.tm_year = ((dt & 0b1111100000000000000000000000000) >> 26) + 2000 - 1900; // 5 bits year (22)
        tm.tm_mon = ((dt & 0b11110000000000000000000000) >> 22) - 1;                 // 4 bits month
        tm.tm_mday = ((dt & 0b1111100000000000000000) >> 17);                        // 5 bits day
        tm.tm_hour = ((dt & 0b11111000000000000) >> 12);                             // 5 bits hour
        tm.tm_min = ((dt & 0b111111000000) >> 6);                                    // 6 bits minutes
        tm.tm_sec = ((dt & 0b111111));                                               // 6 bits seconds
        tm.tm_isdst = 0;
        liveData->params.setGpsTimeFromCar = mktime(&tm);
      }
      if (liveData->commandRequest.equals("222430")) // LAT/LON/ALT
      {
        // 6224303138B0333627362E3022450000003438B031322733342E33224E000002A1AA
        //       ^^^^
        // Parse latitude
        uint8_t b;
        float val;
        String tmp = "";
        for (uint16_t i = 0; i < 14; i++)
        {
          b = liveData->hexToDecFromResponse(6 + (i * 2), 6 + (i * 2) + 2, 1, false);
          if (b == 0)
            break;
          tmp += char((b > 100) ? 95 : b);
        }
        // 18_36'6.0"E
        val = convertLatLonToDecimal(tmp);
        if (val != 0)
          liveData->params.gpsLon = val;
        // Parse logitude
        tmp = "";
        for (uint16_t i = 0; i < 14; i++)
        {
          b = liveData->hexToDecFromResponse(34 + (i * 2), 34 + (i * 2) + 2, 1, false);
          if (b == 0)
            break;
          tmp += char((b > 100) ? 95 : b);
        }
        // 48_12'34.3"N
        val = convertLatLonToDecimal(tmp);
        if (val != 0)
          liveData->params.gpsLat = val;
        // altitude
        int16_t alt = liveData->hexToDecFromResponse(62, 66, 2, false) - 501;
        if (alt > -500)
          liveData->params.gpsAlt = alt;
      }
      if (liveData->commandRequest.equals("222431"))
      {
        // 622431041010
        //           ^^
        liveData->params.gpsSat = liveData->hexToDecFromResponse(10, 12, 1, false); // Satelites
        liveData->params.gpsValid = (liveData->params.gpsSat >= 4);
      }
    }
  }

  // ATSH000746
  if (liveData->currentAtshRequest.equals("ATSH746")) // For data after this header
  {
    if (liveData->commandRequest.equals("222613")) // Inside temperature, °C
    {
      liveData->params.indoorTemperature = liveData->hexToDecFromResponse(6, 10, 2, false) / 5 - 40; // Interior temperature
    }
    if (liveData->commandRequest.equals("222609")) // Outdoor temperature, °C
    {
      liveData->params.outdoorTemperature = liveData->hexToDecFromResponse(6, 8, 1, false) / 2 - 50; // Outdoor temperature
    }
    if (liveData->commandRequest.equals("22263B")) // Recirculation of air, XX=00 -> fresh air, XX=04 -> manual recirculation
    {
      // Put code here to parse the data
    }
    if (liveData->commandRequest.equals("2242DB")) // CO2 content interior, ppm
    {
      // Put code here to parse the data
    }
    if (liveData->commandRequest.equals("22F449")) // Accelerator pedal position, %
    {
      // Put code here to parse the data
    }
  }

  // ATSH000710
  if (liveData->currentAtshRequest.equals("ATSH710")) // For data after this header
  {
    if (liveData->commandRequest.equals("222AB2")) // // HV battery max energy content Wh
    {
      liveData->params.batMaxEnergyContent = liveData->hexToDecFromResponse(6, 14, 4, false) / 1310.77;
      // https://www.goingelectric.de/forum/viewtopic.php?f=97&t=57429&start=20
    }

    if (liveData->commandRequest.equals("222AB8")) // HV battery energy content
    {
      // syslog->println(liveData->commandRequest);
      // syslog->println(liveData->responseRowMerged);
    }

    if (liveData->commandRequest.equals("222AF7")) // // 12V multiframe
    {
      if (liveData->settings.voltmeterEnabled == 0)
      {
        liveData->params.auxVoltage = liveData->hexToDecFromResponse(6, 10, 2, false) / 1024 + 4.26; // 12V multiframe
      }
    }
  }
}

/**
   Is command from queue allowed for execute, or continue with next
*/
bool CarVWID3::commandAllowed()
{
  /* syslog->print("Command allowed: ");
    syslog->print(liveData->currentAtshRequest);
    syslog->print(" ");
    syslog->println(liveData->commandRequest);*/

  // SleepMode Queue Filter
  /*if (liveData->params.sleepModeQueue)
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
  }*/

  // TPMS (once per 30 secs.)
  /*if (liveData->commandRequest.equals("ATSH7A0"))
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
  }*/

  // Disabled command optimizer (allows to log all car values to sdcard, but it's slow)
  if (liveData->settings.disableCommandOptimizer) {
    return true;
  }

  // BMS (only for SCREEN_CELLS)
  if (liveData->currentAtshRequest.equals("ATSH17FC007B"))
  {
    if (liveData->params.displayScreen != SCREEN_CELLS && liveData->params.displayScreenAutoMode != SCREEN_CELLS)
      if (
          liveData->commandRequest.indexOf("22742") == 0 // 22742x => 227425, 227426
          ||
          (liveData->commandRequest.indexOf("221E") == 0 && // All other cells
           liveData->commandRequest.indexOf("221E0") != 0 &&
           liveData->commandRequest.indexOf("221E1") != 0 &&
           liveData->commandRequest.indexOf("221E3") != 0))
      {
        return false;
      }
  }

  // GPS
  if (liveData->currentAtshRequest.equals("ATSH767"))
  {
    if (liveData->settings.gpsHwSerialPort == 255)
    {
      // Sync time from GPS only once, then continue with RTC
      if (liveData->commandRequest.equals("2222B3") && liveData->params.currTimeSyncWithGps)
        return false;
    }
    else
    {
      // Skip all gps command when using external GPS
      return false;
    }
  }

  // HUD speedup
  /*if (liveData->params.displayScreen == SCREEN_HUD)
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
  }*/

  return true;
}

/**
   loadTestData
*/
void CarVWID3::loadTestData()
{
  // MEB GPS  TEST DATA
  liveData->currentAtshRequest = "ATSH767";
  liveData->commandRequest = "222431";
  liveData->responseRowMerged = "622431041010";
  parseRowMerged();
  liveData->commandRequest = "222430";
  liveData->responseRowMerged = "6224303138B0333627362E3022450000003438B031322733342E33224E000002A1AA";
  parseRowMerged();
  liveData->commandRequest = "2222B3";
  liveData->responseRowMerged = "6222B30100585F1792AAAAAAAA";
  parseRowMerged();

  // CELLS
  liveData->currentAtshRequest = "ATSH17FC007B";
  for (uint16_t i = 64; i <= 171; i++)
  {
    liveData->commandRequest = "221E";
    if (i < 16)
      liveData->commandRequest += "0";
    liveData->commandRequest += String(i, HEX);
    liveData->commandRequest.toUpperCase();
    liveData->responseRowMerged = (i >= 160) ? "621EA00FFE" : "621E400A3B";
    parseRowMerged();
  }

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
  liveData->params.batModuleTempC[2] = 28;
  liveData->params.batModuleTempC[3] = 30;
  liveData->params.batModuleTempC[4] = 28;
  liveData->params.batModuleTempC[5] = 29;
  liveData->params.batModuleTempC[6] = 28;
  liveData->params.batModuleTempC[7] = 30;
  liveData->params.batModuleTempC[8] = 28;
  liveData->params.batModuleTempC[9] = 29;
  liveData->params.batModuleTempC[10] = 28;
  liveData->params.batModuleTempC[11] = 30;
  liveData->params.batModuleTempC[12] = 28;
  liveData->params.batModuleTempC[13] = 29;
  liveData->params.batModuleTempC[14] = 28;
  liveData->params.batModuleTempC[15] = 30;

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
void CarVWID3::testHandler(const String &cmd)
{
  int8_t idx = cmd.indexOf("/");
  if (idx == -1)
    return;
  String key = cmd.substring(0, idx);
  String value = cmd.substring(idx + 1);

  // AIRCON SCANNER
  if (key.equals("aircon"))
  {
    // SET TESTER PRESENT
    commInterface->sendPID(liveData->hexToDec("0736", 2, false), "3E");
    delay(10);
    for (uint16_t i = 0; i < (liveData->rxTimeoutMs / 20); i++)
    {
      if (commInterface->receivePID() != 0xff)
        break;
      delay(20);
    }
    delay(liveData->delayBetweenCommandsMs);

    // CHANGE SESSION
    commInterface->sendPID(liveData->hexToDec("0736", 2, false), "1003");
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

    // test=aircon/1
    for (uint16_t a = 0; a < 255; a++)
    {
      syslog->print("NEW CYCLE: ");
      syslog->println(a);
      for (uint16_t b = 240; b < 241; b++)
      {
        String command = "2F";
        if (b < 16)
          command += "0";
        command += String(b, HEX);
        if (a < 16)
          command += "0";
        command += String(a, HEX);
        command.toUpperCase();
        command += "00";

        // EXECUTE COMMAND
        // syslog->print(".");
        commInterface->sendPID(liveData->hexToDec("0736", 2, false), command);
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
    for (uint16_t i = 0; i < 250; i++)
    {
      String command = "2F";
      if (i < 16)
        command += "0";
      command += String(i, HEX);
      command.toUpperCase();
      command += "0100";

      syslog->print(command);
      syslog->print(" ");

      VWID3CarControl(liveData->hexToDec("07B3", 2, false), command);
    }
  }
  // ONE COMMAND
  else
  {
    // test=07C6/2FB00103
    VWID3CarControl(liveData->hexToDec(key, 2, false), value);
  }
}

/**
 * Custom menu
 */
std::vector<String> CarVWID3::customMenu(int16_t menuId)
{
  if (menuId == MENU_CAR_COMMANDS)
    return {
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
void CarVWID3::carCommand(const String &cmd)
{
  if (cmd.equals("doorsUnlock"))
  {
    VWID3CarControl(0x770, "2FBC1103");
  }
  if (cmd.equals("doorsLock"))
  {
    VWID3CarControl(0x770, "2FBC1003");
  }
  if (cmd.equals("chargeCableLockOff"))
  {
    VWID3CarControl(0x770, "2FBC4103");
  }
  if (cmd.equals("chargeCableLockOn"))
  {
    VWID3CarControl(0x770, "2FBC3F03");
  }
  if (cmd.equals("roomLampOff"))
  {
    VWID3CarControl(0x7A0, "2FB01900");
  }
  if (cmd.equals("roomLampOn"))
  {
    VWID3CarControl(0x7A0, "2FB01903");
  }
  if (cmd.equals("luggageLampOff"))
  {
    VWID3CarControl(0x770, "2FBC1C00");
  }
  if (cmd.equals("luggageLampOn"))
  {
    VWID3CarControl(0x770, "2FBC1C03");
  }
  if (cmd.equals("mirrorsUnfold"))
  {
    VWID3CarControl(0x7A0, "2FB05C03");
  }
  if (cmd.equals("mirrorsFold"))
  {
    VWID3CarControl(0x7A0, "2FB05B03");
  }
  if (cmd.equals("heatSteeringWheelOff"))
  {
    VWID3CarControl(0x7A0, "2FB05900"); // heat power
    VWID3CarControl(0x7A0, "2FB05A00"); // LED indicator
  }
  if (cmd.equals("heatSteeringWheelOn"))
  {
    VWID3CarControl(0x7A0, "2FB05903"); // heat power
    VWID3CarControl(0x7A0, "2FB05A03"); // LED indicator
  }
  if (cmd.equals("clusterIndicatorsOff"))
  {
    VWID3CarControl(0x7C6, "2FB00100");
  }
  if (cmd.equals("clusterIndicatorsOn"))
  {
    VWID3CarControl(0x7C6, "2FB00103");
  }
  if (cmd.equals("turnSignalLeftOff"))
  {
    VWID3CarControl(0x770, "2FBC1500");
  }
  if (cmd.equals("turnSignalLeftOn"))
  {
    VWID3CarControl(0x770, "2FBC1503");
  }
  if (cmd.equals("turnSignalRightOff"))
  {
    VWID3CarControl(0x770, "2FBC1600");
  }
  if (cmd.equals("turnSignalRightOn"))
  {
    VWID3CarControl(0x770, "2FBC1603");
  }
  if (cmd.equals("headLightLowOff"))
  {
    VWID3CarControl(0x770, "2FBC0100");
  }
  if (cmd.equals("headLightLowOn"))
  {
    VWID3CarControl(0x770, "2FBC0103");
  }
  if (cmd.equals("headLightHighOff"))
  {
    VWID3CarControl(0x770, "2FBC0200");
  }
  if (cmd.equals("headLightHighOn"))
  {
    VWID3CarControl(0x770, "2FBC0203");
  }
  if (cmd.equals("frontFogLightOff"))
  {
    VWID3CarControl(0x770, "2FBC0300");
  }
  if (cmd.equals("frontFogLightOn"))
  {
    VWID3CarControl(0x770, "2FBC0303");
  }
  if (cmd.equals("rearLightOff"))
  {
    VWID3CarControl(0x770, "2FBC0400");
  }
  if (cmd.equals("rearLightOn"))
  {
    VWID3CarControl(0x770, "2FBC0403");
  }
  if (cmd.equals("rearFogLightOff"))
  {
    VWID3CarControl(0x770, "2FBC0800");
  }
  if (cmd.equals("rearFogLightOn"))
  {
    VWID3CarControl(0x770, "2FBC0803");
  }
  if (cmd.equals("rearDefoggerOff"))
  {
    VWID3CarControl(0x770, "2FBC0C00");
  }
  if (cmd.equals("rearDefoggerOn"))
  {
    VWID3CarControl(0x770, "2FBC0C03");
  }
  if (cmd.equals("rearLeftBrakeLightOff"))
  {
    VWID3CarControl(0x770, "2FBC2B00");
  }
  if (cmd.equals("rearLeftBrakeLightOn"))
  {
    VWID3CarControl(0x770, "2FBC2B03");
  }
  if (cmd.equals("rearRightBrakeLightOff"))
  {
    VWID3CarControl(0x770, "2FBC2C00");
  }
  if (cmd.equals("rearRightBrakeLightOn"))
  {
    VWID3CarControl(0x770, "2FBC2C03");
  }
}

/**
 * VW ID.3 cmds
 */
void CarVWID3::VWID3CarControl(const uint16_t pid, const String &cmd)
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

/**
 * Convert lat/lon to decimal
 */
float CarVWID3::convertLatLonToDecimal(String orig)
{
  return (orig.substring(0, orig.indexOf("_")).toFloat() +
          (orig.substring(orig.indexOf("_") + 1, orig.indexOf("'")).toFloat() / 60) +
          (orig.substring(orig.indexOf("'") + 1, orig.indexOf("\"")).toFloat() / 3600)) *
         ((orig.charAt(orig.length() - 1) == 'W' || orig.charAt(orig.length() - 1) == 'S') ? -1.0 : 1.0);
}
