#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include <BLEDevice.h>
#include "config.h"
#include <vector>

// SUPPORTED CARS
#define CAR_KIA_ENIRO_2020_64     0
#define CAR_HYUNDAI_KONA_2020_64  1
#define CAR_HYUNDAI_IONIQ_2018    2
#define CAR_KIA_ENIRO_2020_39     3
#define CAR_HYUNDAI_KONA_2020_39  4
#define CAR_RENAULT_ZOE           5
#define CAR_KIA_NIRO_PHEV         6
#define CAR_BMW_I3_2014           7
#define CAR_DEBUG_OBD2_KIA        999

// 
#define COMM_TYPE_OBD2BLE4  0
#define COMM_TYPE_OBD2CAN   1
#define COMM_TYPE_OBD2BT3   2

// SCREENS
#define SCREEN_BLANK  0
#define SCREEN_AUTO   1
#define SCREEN_DASH   2
#define SCREEN_SPEED  3
#define SCREEN_CELLS  4
#define SCREEN_CHARGING 5
#define SCREEN_SOC10  6

// 
#define MONTH_SEC     2678400

// Structure with realtime values
typedef struct {
  // System
  time_t currentTime;
  time_t chargingStartTime;
  time_t automaticShutdownTimer;
  // SIM
  time_t lastDataSent;
  bool sim800l_enabled;
  // GPS
  bool currTimeSyncWithGps;
  float gpsLat;
  float gpsLon;
  byte  gpsSat; // satellites count
  int16_t gpsAlt;    
  // SD card
  bool sdcardInit;
  bool sdcardRecording;
  char sdcardFilename[32];
  // Car params
  bool ignitionOn;
  bool chargingOn;
  time_t lastIgnitionOnTime;
  uint64_t operationTimeSec;
  bool sdcardCanNotify;
  bool forwardDriveMode;
  bool reverseDriveMode;
  bool parkModeOrNeutral;
  bool headLights;
  bool dayLights;
  bool brakeLights;
  uint8_t lightInfo;
  uint8_t brakeLightInfo;
  uint8_t espState;
  float batteryTotalAvailableKWh;
  float speedKmh;
  float motorRpm;
  float odoKm;
  float socPerc;
  float socPercPrevious;
  float sohPerc;
  float cumulativeEnergyChargedKWh;
  float cumulativeEnergyChargedKWhStart;
  float cumulativeEnergyDischargedKWh;
  float cumulativeEnergyDischargedKWhStart;
  float availableChargePower; // max regen
  float availableDischargePower; // max power
  float isolationResistanceKOhm;
  float batPowerAmp;
  float batPowerKw;
  float batPowerKwh100;
  float batVoltage;
  float batCellMinV;
  float batCellMaxV;
  float batTempC;
  float batHeaterC;
  float batInletC;
  float batFanStatus;
  float batFanFeedbackHz;
  float batMinC;
  float batMaxC;
  uint16_t batModuleTempCount;
  float batModuleTempC[25];
  float coolingWaterTempC;
  float coolantTemp1C;
  float coolantTemp2C;
  float bmsUnknownTempA;
  float bmsUnknownTempB;
  float bmsUnknownTempC;
  float bmsUnknownTempD;
  float auxPerc;
  float auxCurrentAmp;
  float auxVoltage;
  float indoorTemperature;
  float outdoorTemperature;
  float tireFrontLeftTempC;
  float tireFrontLeftPressureBar;
  float tireFrontRightTempC;
  float tireFrontRightPressureBar;
  float tireRearLeftTempC;
  float tireRearLeftPressureBar;
  float tireRearRightTempC;
  float tireRearRightPressureBar;
  uint16_t cellCount;
  float cellVoltage[98]; // 1..98 has index 0..97
  // Screen - charging graph
  float chargingGraphMinKw[101]; // 0..100% .. Min power Kw
  float chargingGraphMaxKw[101]; // 0..100% .. Max power Kw
  float chargingGraphBatMinTempC[101]; // 0..100% .. Min bat.temp in.C
  float chargingGraphBatMaxTempC[101]; // 0..100% .. Max bat.temp in.C
  float chargingGraphHeaterTempC[101]; // 0..100% .. Heater temp in.C
  float chargingGraphWaterCoolantTempC[101]; // 0..100% .. Heater temp in.C
  // Screen - consumption info
  float soc10ced[11]; // 0..10 (5%, 10%, 20%, 30%, 40%).. (never discharged soc% to 0)
  float soc10cec[11]; // 0..10 (5%, 10%, 20%, 30%, 40%)..
  float soc10odo[11]; // odo history
  time_t soc10time[11]; // time for avg speed
  // additional
  char debugData[256];
  char debugData2[256];
  /*
    uint8_t bmsMainRelay;
    uint8_t highVoltageCharging;
    float inverterCapacitorVoltage;
    float normalChargePort;
    float rapidChargePort;
    ;*/
} PARAMS_STRUC;

// Setting stored to flash
typedef struct {
  byte initFlag; // 183 value
  byte settingsVersion; // current 5
  // === settings version 1
  // =================================
  uint16_t carType; // 0 - Kia eNiro 2020, 1 - Hyundai Kona 2020, 2 - Hyudai Ioniq 2018
  char obdMacAddress[20];
  char serviceUUID[40];
  char charTxUUID[40];
  char charRxUUID[40];
  byte displayRotation; // 0 portrait, 1 landscape, 2.., 3..
  char distanceUnit; // k - kilometers
  char temperatureUnit; // c - celsius
  char pressureUnit; // b - bar
  // === settings version 3
  // =================================
  byte defaultScreen; // 1 .. 6
  byte lcdBrightness; // 0 - auto, 1 .. 100%
  byte debugScreen; // 0 - off, 1 - on
  byte predrawnChargingGraphs; // 0 - off, 1 - on
  // === settings version 4
  // =================================
  byte commType; // 0 - OBD2 BLE4 adapter, 1 - CAN, 2 - BT3
  // Wifi
  byte wifiEnabled; // 0/1 
  char wifiSsid[32];
  char wifiPassword[32];
  // NTP
  byte ntpEnabled; // 0/1 
  byte ntpTimezone;
  byte ntpDaySaveTime; // 0/1
  // SDcard logging
  byte sdcardEnabled; // 0/1 
  byte sdcardAutstartLog; // 0/1   
  // GPRS SIM800L 
  byte gprsEnabled; // 0/1 
  char gprsApn[64];
  // Remote upload
  byte remoteUploadEnabled; // 0/1 
  char remoteApiUrl[64];
  char remoteApiKey[32];
  //
  byte headlightsReminder;
  // === settings version 5
  // =================================
  byte gpsHwSerialPort; // 255-off, 0,1,2 - hw serial
  byte gprsHwSerialPort; // 255-off, 0,1,2 - hw serial
  // === settings version 6
  // =================================
  byte serialConsolePort; // 255-off, 0 - hw serial (std)
  byte debugLevel; // 0 - info only, 1 - debug communication (BLE/CAN), 2 - debug GSM, 3 - debug SDcard
  uint16_t sdcardLogIntervalSec; // every x seconds
  uint16_t gprsLogIntervalSec; // every x seconds
  
  
  //  
} SETTINGS_STRUC;


//
class LiveData {
  protected:
  public:
    // Command loop
    uint16_t commandQueueCount;
    uint16_t commandQueueLoopFrom;
    String commandQueue[300];
    String responseRow;
    String responseRowMerged;
    std::vector<uint8_t> vResponseRowMerged;
    uint16_t commandQueueIndex;
    bool canSendNextAtCommand = false;
    String commandRequest = "";
    String currentAtshRequest = "";
    // Menu
    bool menuVisible = false;
    uint8_t  menuItemsCount;
    uint16_t menuCurrent = 0;
    uint8_t  menuItemSelected = 0;
    uint8_t  menuItemOffset = 0;
    uint16_t scanningDeviceIndex = 0;
    MENU_ITEM* menuItems;

    // Comm
    boolean commConnected = true;
    // Bluetooth4
    boolean bleConnect = true;
    BLEAddress *pServerAddress;
    BLERemoteCharacteristic* pRemoteCharacteristic;
    BLERemoteCharacteristic* pRemoteCharacteristicWrite;
    BLEAdvertisedDevice* foundMyBleDevice;
    BLEClient* pClient;
    BLEScan* pBLEScan;
  
    // Canbus
    uint8_t rxBuffOffset = 0;  // offset of processing received data, in some cars needs to be set to 1, like in BMW i3
    uint8_t expectedMinimalPacketLength = 0;  // what length of packet should be accepted. Set to 0 to accept any length
    
    // Params
    PARAMS_STRUC params;     // Realtime sensor values
    // Settings
    SETTINGS_STRUC settings, tmpSettings; // Settings stored into flash

    //
    void initParams();
    float hexToDec(String hexString, byte bytes = 2, bool signedNum = true);
    float hexToDecFromResponse(byte from, byte to, byte bytes = 2, bool signedNum = true);
    float decFromResponse(byte from, byte to, char **str_end = 0, int base = 16);
    float km2distance(float inKm);
    float celsius2temperature(float inCelsius);
    float bar2pressure(float inBar);
};
