#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <WString.h>
#include <string.h>
#include <sys/time.h>
#include <BLEDevice.h>
#include "config.h"
#include "LogSerial.h"
#include <vector>

// SUPPORTED CARS
#define CAR_KIA_ENIRO_2020_64 0
#define CAR_HYUNDAI_KONA_2020_64 1
#define CAR_HYUNDAI_IONIQ_2018 2
#define CAR_KIA_ENIRO_2020_39 3
#define CAR_HYUNDAI_KONA_2020_39 4
#define CAR_RENAULT_ZOE 5
#define CAR_KIA_NIRO_PHEV 6
#define CAR_BMW_I3_2014 7
#define CAR_KIA_ESOUL_2020_64 8
#define CAR_VW_ID3_2021_45 9
#define CAR_VW_ID3_2021_58 10
#define CAR_VW_ID3_2021_77 11
#define CAR_HYUNDAI_IONIQ5_58 12
#define CAR_HYUNDAI_IONIQ5_72 13
#define CAR_PEUGEOT_E208 14
#define CAR_DEBUG_OBD2_KIA 999

// COMM TYPE
#define COMM_TYPE_OBD2BLE4 0
#define COMM_TYPE_OBD2CAN 1
#define COMM_TYPE_OBD2BT3 2

// REMOTE_UPLOAD
#define REMOTE_UPLOAD_SIM800L 0
#define REMOTE_UPLOAD_WIFI 1

// SCREENS
#define SCREEN_BLANK 0
#define SCREEN_AUTO 1
#define SCREEN_DASH 2
#define SCREEN_SPEED 3
#define SCREEN_CELLS 4
#define SCREEN_CHARGING 5
#define SCREEN_SOC10 6
#define SCREEN_HUD 7

// Battery management mode (liquid)
#define BAT_MAN_MODE_NOT_IMPLEMENTED -1
#define BAT_MAN_MODE_UNKNOWN 0
#define BAT_MAN_MODE_PTC_HEATER 1
#define BAT_MAN_MODE_LOW_TEMPERATURE_RANGE 2
#define BAT_MAN_MODE_COOLING 3
#define BAT_MAN_MODE_OFF 4
#define BAT_MAN_MODE_LOW_TEMPERATURE_RANGE_COOLING 5

//
#define MONTH_SEC 2678400

extern LogSerial *syslog;

// Structure with realtime values
typedef struct
{
  // System
  bool booting;
  time_t currentTime;
  time_t chargingStartTime;
  uint32_t queueLoopCounter;
  bool stopCommandQueue; // sleep mode/screen only & ina3221 voltmeter based
  time_t lastCanbusResponseTime;
  bool ntpTimeSet;
  // Network
  time_t lastDataSent;
  bool sim800l_enabled;
  time_t sim800l_lastOkReceiveTime;
  time_t sim800l_lastOkSendTime;
  bool isWifiBackupLive;
  time_t wifiLastConnectedTime;
  time_t wifiBackupUptime;
  // GPS
  bool currTimeSyncWithGps;
  float gpsLat;
  float gpsLon;
  uint8_t gpsSat; // satellites count
  int16_t gpsAlt;
  // SD card
  bool sdcardInit;
  bool sdcardRecording;
  char sdcardFilename[32];
  // Display
  uint8_t displayScreen;
  uint8_t displayScreenAutoMode;
  time_t lastButtonPushedTime;
  int8_t lcdBrightnessCalc;
  bool spriteInit;
  //voltagemeter INA3221
  time_t lastVoltageReadTime;
  time_t lastVoltageOkTime;
  // Car params
  bool sleepModeQueue;
  bool getValidResponse;
  time_t wakeUpTime;
  bool ignitionOn;
  bool chargingOn;
  bool chargerACconnected;
  bool chargerDCconnected;
  time_t lastIgnitionOnTime;
  time_t lastChargingOnTime;
  uint64_t operationTimeSec;
  bool sdcardCanNotify;
  time_t timeInForwardDriveMode;
  bool forwardDriveMode;
  bool reverseDriveMode;
  bool parkModeOrNeutral;
  bool headLights;
  bool autoLights;
  bool dayLights;
  bool brakeLights;
  bool trunkDoorOpen;
  bool leftFrontDoorOpen;
  bool rightFrontDoorOpen;
  bool leftRearDoorOpen;
  bool rightRearDoorOpen;
  bool hoodDoorOpen;
  float batteryTotalAvailableKWh;
  float speedKmh;
  float speedKmhGPS;
  float avgSpeedKmh;
  float motorRpm;
  float odoKm;
  float odoKmStart;
  float socPercBms;
  float socPerc;
  float socPercPrevious;
  float sohPerc;
  float cumulativeEnergyChargedKWh;
  float cumulativeEnergyChargedKWhStart;
  float cumulativeEnergyDischargedKWh;
  float cumulativeEnergyDischargedKWhStart;
  float availableChargePower;    // max regen
  float availableDischargePower; // max power
  float isolationResistanceKOhm;
  float batPowerAmp;
  float batPowerKw;
  float batPowerKwh100;
  float batVoltage;
  float batCellMinV;
  float batCellMaxV;
  uint8_t batCellMinVNo;
  uint8_t batCellMaxVNo;
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
  float auxTemperature;
  int8_t batteryManagementMode;

  // MCU
  float inverterTempC;
  float motorTempC;

  // HVAC
  float indoorTemperature;
  float outdoorTemperature;
  float evaporatorTempC;
  float tireFrontLeftTempC;
  float tireFrontLeftPressureBar;
  float tireFrontRightTempC;
  float tireFrontRightPressureBar;
  float tireRearLeftTempC;
  float tireRearLeftPressureBar;
  float tireRearRightTempC;
  float tireRearRightPressureBar;
  uint16_t cellCount;
  float cellVoltage[108]; // 1..108 has index 0..107

  // Screen - charging graph
  float chargingGraphMinKw[101];             // 0..100% .. Min power Kw
  float chargingGraphMaxKw[101];             // 0..100% .. Max power Kw
  float chargingGraphBatMinTempC[101];       // 0..100% .. Min bat.temp in.C
  float chargingGraphBatMaxTempC[101];       // 0..100% .. Max bat.temp in.C
  float chargingGraphHeaterTempC[101];       // 0..100% .. Heater temp in.C
  float chargingGraphWaterCoolantTempC[101]; // 0..100% .. Heater temp in.C

  // Screen - consumption info
  float soc10ced[11];   // 0..10 (5%, 10%, 20%, 30%, 40%).. (never discharged soc% to 0)
  float soc10cec[11];   // 0..10 (5%, 10%, 20%, 30%, 40%)..
  float soc10odo[11];   // odo history
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
typedef struct
{
  uint8_t initFlag;        // 183 value
  uint8_t settingsVersion; // see bellow for latest version
  // === settings version 1
  // =================================
  uint16_t carType; // 0 - Kia eNiro 2020, 1 - Hyundai Kona 2020, 2 - Hyudai Ioniq 2018
  char obdMacAddress[20];
  char serviceUUID[40];
  char charTxUUID[40];
  char charRxUUID[40];
  uint8_t displayRotation; // 0 portrait, 1 landscape, 2.., 3..
  char distanceUnit;    // k - kilometers
  char temperatureUnit; // c - celsius
  char pressureUnit;    // b - bar
  // === settings version 3
  // =================================
  uint8_t defaultScreen;          // 1 .. 6
  uint8_t lcdBrightness;          // 0 - auto, 1 .. 100%
  uint8_t voltmeterEnabled;       // I2C Voltmeter INA3221; 0 - off, 1 - on
  uint8_t predrawnChargingGraphs; // 0 - off, 1 - on
  // === settings version 4
  // =================================
  uint8_t commType; // 0 - OBD2 BLE4 adapter, 1 - CAN, 2 - BT3
  // Wifi
  uint8_t wifiEnabled; // 0/1
  char wifiSsid[32];
  char wifiPassword[32];
  // NTP
  uint8_t ntpEnabled; // 0/1
  uint8_t ntpTimezone;
  uint8_t ntpDaySaveTime; // 0/1
  // SDcard logging
  uint8_t sdcardEnabled;     // 0/1
  uint8_t sdcardAutstartLog; // 0/1
  // GPRS SIM800L
  uint8_t gprsEnabled_deprecated; // Deprecated - Variable is free to use
  char gprsApn[64];            // Will be used as mqtt server url in future builds
  // Remote upload
  uint8_t remoteUploadEnabled_deprecated; // Deprecated - Variable is free to use
  char remoteApiUrl[64];               // Will be used as mqtt password in future builds
  char remoteApiKey[32];               // Will be used as mqtt username in future builds
  //
  uint8_t headlightsReminder;
  // === settings version 5
  // =================================
  uint8_t gpsHwSerialPort;  // 255-off, 0,1,2 - hw serial
  uint8_t gprsHwSerialPort; // 255-off, 0,1,2 - hw serial
  // === settings version 6
  // =================================
  uint8_t serialConsolePort;        // 255-off, 0 - hw serial (std)
  uint8_t debugLevel;            // 0 - info only, 1 - debug communication (BLE/CAN), 2 - debug GSM, 3 - debug SDcard, 4 - GPS
  uint16_t sdcardLogIntervalSec; // every x seconds
  uint16_t gprsLogIntervalSec;   // every x seconds
  // === settings version 7
  // =================================
  uint8_t sleepModeLevel; // 0 - off, 1 - screen only, 2 - deep sleep
  // === settings version 8
  // =================================
  float voltmeterCutOff;
  float voltmeterSleep;
  float voltmeterWakeUp;
  uint8_t voltmeterBasedSleep; // 0 - off, 1 - on
  // === settings version 9
  // =================================
  uint16_t remoteUploadIntervalSec; // Send data to remote server every X seconds (0 = disabled) // will be used as mqtt upload interval in future builds
  uint16_t sleepModeIntervalSec;    // In sleep, check CANbus / Volmeter every X seconds
  uint16_t sleepModeShutdownHrs;    // 0 - disabled # shutdown after X hours of sleep
  uint16_t remoteUploadModuleType;  // 0 (REMOTE_UPLOAD_SIM800L) - SIM800L, 1 - wifi
  // == settings version 10
  // =================================
  uint16_t remoteUploadAbrpIntervalSec; // Send data to ABRP API every X seconds (0 = disabled)
  char abrpApiToken[48]; // ABRP APIkey
  // == settings version 11
  // =================================
  int8_t timezone;                  // 0 - default, -11 .. +14 hrs
  uint8_t daylightSaving;              // 0/1
  uint8_t rightHandDrive;              // 0 - default is LHD, 1 RHD (UK)
  // == settings version 12
  char wifiSsidb[32];                  //backup wifi SSID
  char wifiPasswordb[32];              //backup wifi Pass
  uint8_t backupWifiEnabled;           //enable Backup WIFI fallback 0/1
} SETTINGS_STRUC;

// LiveData class
class LiveData
{
protected:
public:
  // Command loop
  struct Command_t
  {
    uint8_t startChar; // special starting character used by some cars
    String request;
  };

  uint16_t commandQueueCount;
  uint16_t commandQueueLoopFrom;
  std::vector<Command_t> commandQueue;
  String responseRow;
  String responseRowMerged;
  String prevResponseRowMerged;
  std::vector<uint8_t> vResponseRowMerged;
  uint16_t commandQueueIndex;
  bool canSendNextAtCommand = false;
  uint8_t commandStartChar;
  String commandRequest = ""; // TODO: us Command_t struct
  String currentAtshRequest = "";
  // Menu
  bool menuVisible = false;
  uint8_t menuItemsCount;
  uint16_t menuCurrent = 0;
  uint8_t menuItemSelected = 0;
  uint8_t menuItemOffset = 0;
  uint16_t scanningDeviceIndex = 0;
  MENU_ITEM *menuItems;

  // Comm
  boolean commConnected = true;
  // Bluetooth4
  boolean bleConnect = true;
  BLEAddress *pServerAddress;
  BLERemoteCharacteristic *pRemoteCharacteristic;
  BLERemoteCharacteristic *pRemoteCharacteristicWrite;
  BLEAdvertisedDevice *foundMyBleDevice;
  BLEClient *pClient;
  BLEScan *pBLEScan;

  // Canbus
  bool bAdditionalStartingChar = false;    // some cars uses additional starting character in beginning of tx and rx messages
  uint8_t expectedMinimalPacketLength = 0; // what length of packet should be accepted. Set to 0 to accept any length
  uint16_t rxTimeoutMs = 500;              // timeout for receiving of CAN response
  uint16_t delayBetweenCommandsMs = 0;     // delay between commands, set to 0 if no delay is needed

  // Params
  PARAMS_STRUC params; // Realtime sensor values
  // Settings
  SETTINGS_STRUC settings, tmpSettings; // Settings stored into flash

  //
  void initParams();
  double hexToDec(String hexString, uint8_t bytes = 2, bool signedNum = true);
  double hexToDecFromResponse(uint8_t from, uint8_t to, uint8_t bytes = 2, bool signedNum = true);
  float decFromResponse(uint8_t from, uint8_t to, char **str_end = 0, int base = 16);
  float km2distance(float inKm);
  float celsius2temperature(float inCelsius);
  float bar2pressure(float inBar);
  String getBatteryManagementModeStr(int8_t mode);
};
