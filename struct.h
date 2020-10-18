
#define CAR_KIA_ENIRO_2020_64     0
#define CAR_HYUNDAI_KONA_2020_64  1
#define CAR_HYUNDAI_IONIQ_2018    2
#define CAR_KIA_ENIRO_2020_39     3
#define CAR_HYUNDAI_KONA_2020_39  4
#define CAR_DEBUG_OBD2_KIA        999

// Commands loop
uint16_t commandQueueCount;
uint16_t commandQueueLoopFrom;
String commandQueue[300];
String responseRow;
String responseRowMerged;
uint16_t commandQueueIndex;
bool canSendNextAtCommand = false;
String commandRequest = "";
String currentAtshRequest = "";

// Structure with realtime values
typedef struct {
  float batteryTotalAvailableKWh; 
  float speedKmh;
  float odoKm;
  float socPerc;
  float sohPerc;
  float cumulativeEnergyChargedKWh;
  float cumulativeEnergyChargedKWhStart;
  float cumulativeEnergyDischargedKWh;
  float cumulativeEnergyDischargedKWhStart;
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
  float batModuleTempC[12];
  float coolingWaterTempC;
  float coolantTemp1C;
  float coolantTemp2C;
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
  float chargingGraphKw[101]; // 0..100% .. how many HW in each step
  float chargingGraphMinTempC[101]; // 0..100% .. Min temp in.C
  float chargingGraphMaxTempC[101]; // 0..100% .. Max temp in.C
  // Screen - consumption info
  float soc10ced[11]; // 0..10 (5%, 10%, 20%, 30%, 40%).. (never discharged soc% to 0)
  float soc10cec[11]; // 0..10 (5%, 10%, 20%, 30%, 40%)..
  float soc10odo[11]; // odo history
  time_t soc10time[11]; // time for avg speed
} PARAMS_STRUC;

// Setting stored to flash
typedef struct {
  byte initFlag; // 183 value
  byte settingsVersion; // 1
  uint16_t carType; // 0 - Kia eNiro 2020, 1 - Hyundai Kona 2020, 2 - Hyudai Ioniq 2018
  char obdMacAddress[20];
  char serviceUUID[40];
  char charTxUUID[40];
  char charRxUUID[40];
  byte displayRotation; // 0 portrait, 1 landscape, 2.., 3..
  char distanceUnit; // k - kilometers
  char temperatureUnit; // c - celsius
  char pressureUnit; // b - bar
} SETTINGS_STRUC;

PARAMS_STRUC params;     // Realtime sensor values
PARAMS_STRUC oldParams;  // Old states used for change detection (draw optimization)
SETTINGS_STRUC settings, tmpSettings; // Settings stored into flash

/**
  Hex to dec (1-2 byte values, signed/unsigned)
  For 4 byte change int to long and add part for signed numbers
*/
float hexToDec(String hexString, byte bytes = 2, bool signedNum = true) {

  unsigned int decValue = 0;
  unsigned int nextInt;

  for (int i = 0; i < hexString.length(); i++) {
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
  }

  // Unsigned - do nothing
  if (!signedNum) {
    return decValue;
  }
  // Signed for 1, 2 bytes
  if (bytes == 1) {
    return (decValue > 127 ? (float)decValue - 256.0 : decValue);
  }
  return (decValue > 32767 ? (float)decValue - 65536.0 : decValue);
}
