#define ARDUINOJSON_USE_LONG_LONG 1

#include <WiFi.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include "BoardInterface.h"
#include "CommObd2Bt3.h"
#include "CommObd2Ble4.h"
#include "CommObd2Can.h"
#include "CommObd2Wifi.h"
#include "LiveData.h"
#include "Solarlib.h"

/**
 * Set live data
 */
void BoardInterface::setLiveData(LiveData *pLiveData)
{
  liveData = pLiveData;
}

/**
 * Attach car interface
 */
void BoardInterface::attachCar(CarInterface *pCarInterface)
{
  carInterface = pCarInterface;
}

/**
 * Shutdown device
 */
void BoardInterface::shutdownDevice()
{
  syslog->println("Shutdown.");

  char msg[20];
  for (int i = 3; i >= 1; i--)
  {
    sprintf(msg, "Shutdown in %d sec.", i);
    displayMessage(msg, "");
    delay(1000);
  }

  setCpuFrequencyMhz(80);
  turnOffScreen();
  // WiFi.disconnect(true);
  // WiFi.mode(WIFI_OFF);

  commInterface->disconnectDevice();
  // adc_power_off();
  // esp_wifi_stop();
  esp_bt_controller_disable();

  delay(2000);
  // esp_sleep_enable_timer_wakeup(525600L * 60L * 1000000L); // minutes
  esp_deep_sleep_start();
}

/**
 * Save setting to flash memory
 */
void BoardInterface::saveSettings()
{
  syslog->println("Settings saved to eeprom.");
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();
}

/**
 * Reset settings (factory reset)
 */
void BoardInterface::resetSettings()
{
  syslog->println("Factory reset.");
  liveData->settings.initFlag = 1;
  EEPROM.put(0, liveData->settings);
  EEPROM.commit();

  displayMessage("Settings erased", "Restarting in 5 secs.");

  delay(5000);
  ESP.restart();
}

/**
 * Load setting from flash memory, upgrade structure if version differs
 */
void BoardInterface::loadSettings()
{
  String tmpStr;

  // Default settings
  liveData->settings.initFlag = 183;
  liveData->settings.settingsVersion = 16;
  liveData->settings.carType = CAR_KIA_ENIRO_2020_64;
  tmpStr = "00:00:00:00:00:00"; // Pair via menu (middle button)
  tmpStr.toCharArray(liveData->settings.obdMacAddress, tmpStr.length() + 1);
  tmpStr = "000018f0-0000-1000-8000-00805f9b34fb"; // Default UUID's for VGate iCar Pro BLE4 adapter
  tmpStr.toCharArray(liveData->settings.serviceUUID, tmpStr.length() + 1);
  tmpStr = "00002af0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(liveData->settings.charTxUUID, tmpStr.length() + 1);
  tmpStr = "00002af1-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(liveData->settings.charRxUUID, tmpStr.length() + 1);
  liveData->settings.displayRotation = 1; // 1,3
  liveData->settings.distanceUnit = 'k';
  liveData->settings.temperatureUnit = 'c';
  liveData->settings.pressureUnit = 'b';
  liveData->settings.defaultScreen = 1;
  liveData->settings.lcdBrightness = 0;
  liveData->settings.predrawnChargingGraphs = 1;
  // liveData->settings.commType = COMM_TYPE_OBD2_BLE4; // BLE4
  liveData->settings.commType = COMM_TYPE_CAN_COMMU; // CAN
  liveData->settings.wifiEnabled = 0;
  tmpStr = "empty";
  tmpStr.toCharArray(liveData->settings.wifiSsid, tmpStr.length() + 1);
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.wifiPassword, tmpStr.length() + 1);
  liveData->settings.ntpEnabled = 1;
  liveData->settings.ntpTimezone = 1;
  liveData->settings.ntpDaySaveTime = 0;
  liveData->settings.sdcardEnabled = 0;
  liveData->settings.sdcardAutstartLog = 1;
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.gprsApn, tmpStr.length() + 1);
  // Remote upload
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.remoteApiUrl, tmpStr.length() + 1);
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.remoteApiKey, tmpStr.length() + 1);
  liveData->settings.headlightsReminder = 0;
  liveData->settings.gpsHwSerialPort = 255;  // off
  liveData->settings.gprsHwSerialPort = 255; // off
  liveData->settings.serialConsolePort = 0;  // hwuart0
  liveData->settings.debugLevel = 1;         // 0 - info only, 1 - debug communication (BLE/CAN), 2 - debug GSM, 3 - debug SDcard
  liveData->settings.sdcardLogIntervalSec = 2;
  liveData->settings.gprsLogIntervalSec = 60;
  liveData->settings.sleepModeLevel = SLEEP_MODE_OFF;
  liveData->settings.voltmeterEnabled = 0;
  liveData->settings.voltmeterBasedSleep = 0;
  liveData->settings.voltmeterCutOff = 12.0;
  liveData->settings.voltmeterSleep = 12.8;
  liveData->settings.voltmeterWakeUp = 13.0;
  liveData->settings.remoteUploadIntervalSec = 60;
  liveData->settings.sleepModeIntervalSec = 30;
  liveData->settings.sleepModeShutdownHrs = 72;
  liveData->settings.remoteUploadModuleType = REMOTE_UPLOAD_SIM800L;
  liveData->settings.remoteUploadAbrpIntervalSec = 0;
  tmpStr = "empty";
  tmpStr.toCharArray(liveData->settings.abrpApiToken, tmpStr.length() + 1);
  // v11
  liveData->settings.timezone = 0;
  liveData->settings.daylightSaving = 0;
  liveData->settings.rightHandDrive = 0;
  // v12
  tmpStr = "empty";
  tmpStr.toCharArray(liveData->settings.wifiSsid2, tmpStr.length() + 1);
  tmpStr = "not_set";
  tmpStr.toCharArray(liveData->settings.wifiPassword2, tmpStr.length() + 1);
  liveData->settings.backupWifiEnabled = 0;
  // v13
  liveData->settings.threading = 0;
  liveData->settings.speedCorrection = 0;
  // v14
  liveData->settings.disableCommandOptimizer = 0;
  // v15
  liveData->settings.abrpSdcardLog = 0;
  // v16
  tmpStr = "OBD2"; // default bt3 obd2 adapter name
  tmpStr.toCharArray(liveData->settings.obd2Name, tmpStr.length() + 1);
  tmpStr = "192.168.0.10"; // obd2wifi adapter ip
  tmpStr.toCharArray(liveData->settings.obd2WifiIp, tmpStr.length() + 1);
  liveData->settings.obd2WifiPort = 35000;

  // Load settings and replace default values
  syslog->println("Reading settings from eeprom.");
  EEPROM.begin(sizeof(SETTINGS_STRUC));
  EEPROM.get(0, liveData->tmpSettings);

  // Init flash with default settings
  if (liveData->tmpSettings.initFlag != 183)
  {
    syslog->println("Settings not found. Initialization.");
    saveSettings();
  }
  else
  {
    syslog->print("Loaded settings ver.: ");
    syslog->println(liveData->tmpSettings.settingsVersion);

    // Upgrade structure
    if (liveData->settings.settingsVersion != liveData->tmpSettings.settingsVersion)
    {
      if (liveData->tmpSettings.settingsVersion == 1)
      {
        liveData->tmpSettings.settingsVersion = 2;
        liveData->tmpSettings.defaultScreen = liveData->settings.defaultScreen;
        liveData->tmpSettings.lcdBrightness = liveData->settings.lcdBrightness;
      }
      if (liveData->tmpSettings.settingsVersion == 2)
      {
        liveData->tmpSettings.settingsVersion = 3;
        liveData->tmpSettings.predrawnChargingGraphs = liveData->settings.predrawnChargingGraphs;
      }
      if (liveData->tmpSettings.settingsVersion == 3)
      {
        liveData->tmpSettings.settingsVersion = 4;
        liveData->tmpSettings.commType = COMM_TYPE_OBD2_BLE4; // BLE4
        liveData->tmpSettings.wifiEnabled = 0;
        tmpStr = "empty";
        tmpStr.toCharArray(liveData->tmpSettings.wifiSsid, tmpStr.length() + 1);
        tmpStr = "not_set";
        tmpStr.toCharArray(liveData->tmpSettings.wifiPassword, tmpStr.length() + 1);
        liveData->tmpSettings.ntpEnabled = 1;
        liveData->tmpSettings.ntpTimezone = 1;
        liveData->tmpSettings.ntpDaySaveTime = 0;
        liveData->tmpSettings.sdcardEnabled = 0;
        liveData->tmpSettings.sdcardAutstartLog = 1;
        tmpStr = "internet.t-mobile.cz";
        tmpStr.toCharArray(liveData->tmpSettings.gprsApn, tmpStr.length() + 1);
        // Remote upload
        tmpStr = "http://api.example.com";
        tmpStr.toCharArray(liveData->tmpSettings.remoteApiUrl, tmpStr.length() + 1);
        tmpStr = "example";
        tmpStr.toCharArray(liveData->tmpSettings.remoteApiKey, tmpStr.length() + 1);
        liveData->tmpSettings.headlightsReminder = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 4)
      {
        liveData->tmpSettings.settingsVersion = 5;
        liveData->tmpSettings.gpsHwSerialPort = 255; // off
      }
      if (liveData->tmpSettings.settingsVersion == 5)
      {
        liveData->tmpSettings.settingsVersion = 6;
        liveData->tmpSettings.serialConsolePort = 0; // hwuart0
        liveData->tmpSettings.debugLevel = 0;        // show all
        liveData->tmpSettings.sdcardLogIntervalSec = 2;
        liveData->tmpSettings.gprsLogIntervalSec = 60;
      }
      if (liveData->tmpSettings.settingsVersion == 6)
      {
        liveData->tmpSettings.settingsVersion = 7;
        liveData->tmpSettings.sleepModeLevel = SLEEP_MODE_OFF;
      }
      if (liveData->tmpSettings.settingsVersion == 7)
      {
        liveData->tmpSettings.settingsVersion = 8;
        liveData->tmpSettings.voltmeterEnabled = 0;
        liveData->tmpSettings.voltmeterBasedSleep = 0;
        liveData->tmpSettings.voltmeterCutOff = 12.0;
        liveData->tmpSettings.voltmeterSleep = 12.8;
        liveData->tmpSettings.voltmeterWakeUp = 13.0;
      }
      if (liveData->tmpSettings.settingsVersion == 8)
      {
        liveData->tmpSettings.settingsVersion = 9;
        liveData->tmpSettings.remoteUploadIntervalSec = 60;
        liveData->tmpSettings.sleepModeIntervalSec = 30;
        liveData->tmpSettings.sleepModeShutdownHrs = 72;
        liveData->tmpSettings.remoteUploadModuleType = REMOTE_UPLOAD_SIM800L;
      }
      if (liveData->tmpSettings.settingsVersion == 9)
      {
        liveData->tmpSettings.settingsVersion = 10;
        liveData->tmpSettings.remoteUploadAbrpIntervalSec = 0;
        tmpStr = "empty";
        tmpStr.toCharArray(liveData->tmpSettings.abrpApiToken, tmpStr.length() + 1);
      }
      if (liveData->tmpSettings.settingsVersion == 10)
      {
        liveData->tmpSettings.settingsVersion = 11;
        liveData->tmpSettings.timezone = 0;
        liveData->tmpSettings.daylightSaving = 0;
        liveData->tmpSettings.rightHandDrive = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 11)
      {
        liveData->tmpSettings.settingsVersion = 12;
        tmpStr = "empty";
        tmpStr.toCharArray(liveData->tmpSettings.wifiSsid2, tmpStr.length() + 1);
        tmpStr = "not_set";
        tmpStr.toCharArray(liveData->tmpSettings.wifiPassword2, tmpStr.length() + 1);
        liveData->tmpSettings.backupWifiEnabled = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 12)
      {
        liveData->tmpSettings.settingsVersion = 13;
        liveData->tmpSettings.threading = 0;
        liveData->tmpSettings.speedCorrection = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 13)
      {
        liveData->tmpSettings.settingsVersion = 14;
        liveData->tmpSettings.disableCommandOptimizer = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 14)
      {
        liveData->tmpSettings.settingsVersion = 15;
        liveData->tmpSettings.abrpSdcardLog = 0;
      }
      if (liveData->tmpSettings.settingsVersion == 15)
      {
        liveData->tmpSettings.settingsVersion = 16;
        tmpStr = "OBD2"; // default bt3 obd2 adapter name
        tmpStr.toCharArray(liveData->tmpSettings.obd2Name, tmpStr.length() + 1);
        tmpStr = "192.168.0.10"; // obd2wifi adapter ip
        tmpStr.toCharArray(liveData->tmpSettings.obd2WifiIp, tmpStr.length() + 1);
        liveData->tmpSettings.obd2WifiPort = 35000;
      }
      if (liveData->tmpSettings.settingsVersion == 16)
      {
        liveData->tmpSettings.settingsVersion = 17;
        liveData->tmpSettings.contributeData = 1;
        tmpStr = "\n"; 
        tmpStr.toCharArray(liveData->tmpSettings.contributeToken, tmpStr.length() + 1);
        liveData->tmpSettings.mqttEnabled = 0;
        tmpStr = "192.168.0.1"; 
        tmpStr.toCharArray(liveData->tmpSettings.mqttServer, tmpStr.length() + 1);
        tmpStr = "evdash"; 
        tmpStr.toCharArray(liveData->tmpSettings.mqttId, tmpStr.length() + 1);
        tmpStr = "evuser"; 
        tmpStr.toCharArray(liveData->tmpSettings.mqttUsername, tmpStr.length() + 1);
        tmpStr = "evpass"; 
        tmpStr.toCharArray(liveData->tmpSettings.mqttPassword, tmpStr.length() + 1);
        tmpStr = "evdash/sensors"; 
        tmpStr.toCharArray(liveData->tmpSettings.mqttPubTopic, tmpStr.length() + 1);
      }

      // Save upgraded structure
      liveData->settings = liveData->tmpSettings;
      saveSettings();
    }

    // Apply settings from flash if needed
    liveData->settings = liveData->tmpSettings;
  }

  syslog->setDebugLevel(liveData->settings.debugLevel);
}

/**
 * After setup
 */
void BoardInterface::afterSetup()
{
  syslog->println("BoardInterface::afterSetup");

  // Init COMM iterface
  syslog->print("Init communication device: ");
  syslog->println(liveData->settings.commType);

  if (liveData->settings.commType == COMM_TYPE_OBD2_BLE4)
  {
    commInterface = new CommObd2Ble4();
  }
  else if (liveData->settings.commType == COMM_TYPE_CAN_COMMU)
  {
    commInterface = new CommObd2Can();
  }
  else if (liveData->settings.commType == COMM_TYPE_OBD2_BT3)
  {
    commInterface = new CommObd2Bt3();
  }
  else if (liveData->settings.commType == COMM_TYPE_OBD2_WIFI)
  {
    commInterface = new CommObd2Wifi();
  }

  // Connect device
  commInterface->initComm(liveData, this);
  commInterface->connectDevice();
  carInterface->setCommInterface(commInterface);
}

/**
 * Custom commands
 */
void BoardInterface::customConsoleCommand(String cmd)
{
  if (cmd.equals("reboot"))
    ESP.restart();
  if (cmd.equals("saveSettings"))
    saveSettings();
  if (cmd.equals("time"))
    showTime();
  if (cmd.equals("ntpSync"))
    ntpSync();
  if (cmd.equals("ipconfig"))
    showNet();
  if (cmd.equals("shutdown"))
    enterSleepMode(0);
  // CAN comparer
  if (cmd.equals("compare"))
    commInterface->compareCanRecords();

  int8_t idx = cmd.indexOf("=");
  if (idx == -1)
    return;

  String key = cmd.substring(0, idx);
  String value = cmd.substring(idx + 1);

  if (key == "serviceUUID")
    value.toCharArray(liveData->settings.serviceUUID, value.length() + 1);
  if (key == "charTxUUID")
    value.toCharArray(liveData->settings.charTxUUID, value.length() + 1);
  if (key == "charRxUUID")
    value.toCharArray(liveData->settings.charRxUUID, value.length() + 1);

  if (key == "obd2ip")
    value.toCharArray(liveData->settings.obd2WifiIp, value.length() + 1);
  if (key == "obd2port")
    liveData->settings.obd2WifiPort = value.toInt();

  if (key == "wifiSsid")
    value.toCharArray(liveData->settings.wifiSsid, value.length() + 1);
  if (key == "wifiPassword")
    value.toCharArray(liveData->settings.wifiPassword, value.length() + 1);
  if (key == "wifiSsid2")
  {
    value.toCharArray(liveData->settings.wifiSsid2, value.length() + 1);
    if (strcmp(liveData->settings.wifiSsid2, "empty") == 0)
    {
      liveData->settings.backupWifiEnabled = 0;
    }
    else
    {
      liveData->settings.backupWifiEnabled = 1;
    }
  }
  if (key == "wifiPassword2")
    value.toCharArray(liveData->settings.wifiPassword2, value.length() + 1);
  if (key == "gprsApn")
    value.toCharArray(liveData->settings.gprsApn, value.length() + 1);
  if (key == "remoteApiUrl")
    value.toCharArray(liveData->settings.remoteApiUrl, value.length() + 1);
  if (key == "remoteApiKey")
    value.toCharArray(liveData->settings.remoteApiKey, value.length() + 1);
  if (key == "abrpApiToken")
    value.toCharArray(liveData->settings.abrpApiToken, value.length() + 1);

  // Mqtt
  if (key == "mqttServer")
    value.toCharArray(liveData->settings.mqttServer, value.length() + 1);
  if (key == "mqttId")
    value.toCharArray(liveData->settings.mqttId, value.length() + 1);
  if (key == "mqttUsername")
    value.toCharArray(liveData->settings.mqttUsername, value.length() + 1);
  if (key == "mqttPassword")
    value.toCharArray(liveData->settings.mqttPassword, value.length() + 1);
  if (key == "mqttPubTopic")
    value.toCharArray(liveData->settings.mqttPubTopic, value.length() + 1);

  // 
  if (key == "debugLevel")
  {
    liveData->settings.debugLevel = value.toInt();
    syslog->setDebugLevel(liveData->settings.debugLevel);
  }
  if (key == "setTime")
  {
    setTime(value);
  }
  // CAN comparer
  if (key == "record")
  {
    commInterface->recordLoop(value.toInt());
  }
  if (key == "test")
  {
    carInterface->testHandler(value);
  }
}

/**
 *  Parser response from obd2/can
 */
void BoardInterface::parseRowMerged()
{
  carInterface->parseRowMerged();
}

/**
 * Serialize parameters for abrp/remote upload/sdcard
 */
bool BoardInterface::serializeParamsToJson(File file, bool inclApiKey)
{
  StaticJsonDocument<4096> jsonData;

  if (inclApiKey)
    jsonData["apiKey"] = liveData->settings.remoteApiKey;

  jsonData["carType"] = liveData->settings.carType;
  jsonData["batTotalKwh"] = liveData->params.batteryTotalAvailableKWh;
  jsonData["currTime"] = liveData->params.currentTime + (liveData->settings.timezone * 3600) + (liveData->settings.daylightSaving * 3600);
  jsonData["opTime"] = liveData->params.operationTimeSec;

  jsonData["gpsSat"] = liveData->params.gpsSat;
  jsonData["lat"] = liveData->params.gpsLat;
  jsonData["lon"] = liveData->params.gpsLon;
  jsonData["alt"] = liveData->params.gpsAlt;
  jsonData["speedKmhGPS"] = liveData->params.speedKmhGPS;

  jsonData["ignitionOn"] = liveData->params.ignitionOn;
  jsonData["chargingOn"] = liveData->params.chargingOn;

  jsonData["socPerc"] = liveData->params.socPerc;
  jsonData["socPercBms"] = liveData->params.socPercBms;
  jsonData["sohPerc"] = liveData->params.sohPerc;
  jsonData["powKwh100"] = liveData->params.batPowerKwh100;
  jsonData["speedKmh"] = liveData->params.speedKmh;
  jsonData["motorRpm"] = liveData->params.motorRpm;
  jsonData["odoKm"] = liveData->params.odoKm;

  if (liveData->params.batEnergyContent != 1)
    jsonData["batEneWh"] = liveData->params.batEnergyContent;
  if (liveData->params.batMaxEnergyContent != 1)
    jsonData["batMaxEneWh"] = liveData->params.batMaxEnergyContent;

  jsonData["batPowKw"] = liveData->params.batPowerKw;
  jsonData["batPowA"] = liveData->params.batPowerAmp;
  jsonData["batV"] = liveData->params.batVoltage;
  jsonData["cecKwh"] = liveData->params.cumulativeEnergyChargedKWh;
  jsonData["cedKwh"] = liveData->params.cumulativeEnergyDischargedKWh;
  jsonData["maxChKw"] = liveData->params.availableChargePower;
  jsonData["maxDisKw"] = liveData->params.availableDischargePower;

  jsonData["cellMinV"] = liveData->params.batCellMinV;
  jsonData["cellMaxV"] = liveData->params.batCellMaxV;
  if (liveData->params.batCellMinVNo != 255)
    jsonData["cellMinVNo"] = liveData->params.batCellMinVNo;
  if (liveData->params.batCellMaxVNo != 255)
    jsonData["cellMaxVNo"] = liveData->params.batCellMaxVNo;
  jsonData["bMinC"] = round(liveData->params.batMinC);
  jsonData["bMaxC"] = round(liveData->params.batMaxC);
  jsonData["bHeatC"] = round(liveData->params.batHeaterC);
  jsonData["bInletC"] = round(liveData->params.batInletC);
  jsonData["bFanSt"] = liveData->params.batFanStatus;
  jsonData["bWatC"] = round(liveData->params.coolingWaterTempC);
  jsonData["tmpA"] = round(liveData->params.bmsUnknownTempA);
  jsonData["tmpB"] = round(liveData->params.bmsUnknownTempB);
  jsonData["tmpC"] = round(liveData->params.bmsUnknownTempC);
  jsonData["tmpD"] = round(liveData->params.bmsUnknownTempD);

  jsonData["invC"] = round(liveData->params.inverterTempC);
  jsonData["motC"] = round(liveData->params.motorTempC);

  jsonData["auxPerc"] = liveData->params.auxPerc;
  jsonData["auxV"] = liveData->params.auxVoltage;
  jsonData["auxA"] = liveData->params.auxCurrentAmp;

  jsonData["inC"] = liveData->params.indoorTemperature;
  jsonData["outC"] = liveData->params.outdoorTemperature;
  jsonData["evapC"] = liveData->params.evaporatorTempC;
  jsonData["c1C"] = liveData->params.coolantTemp1C;
  jsonData["c2C"] = liveData->params.coolantTemp2C;

  jsonData["tFlC"] = liveData->params.tireFrontLeftTempC;
  jsonData["tFlBar"] = round(liveData->params.tireFrontLeftPressureBar * 10) / 10;
  jsonData["tFrC"] = liveData->params.tireFrontRightTempC;
  jsonData["tFrBar"] = round(liveData->params.tireFrontRightPressureBar * 10) / 10;
  jsonData["tRlC"] = liveData->params.tireRearLeftTempC;
  jsonData["tRlBar"] = round(liveData->params.tireRearLeftPressureBar * 10) / 10;
  jsonData["tRrC"] = liveData->params.tireRearRightTempC;
  jsonData["tRrBar"] = round(liveData->params.tireRearRightPressureBar * 10) / 10;
  jsonData["brakeL"] = liveData->params.brakeLights;

  // cell voltage
  for (int i = 0; i < liveData->params.cellCount; i++)
  {
    if (liveData->params.cellVoltage[i] == -1)
      continue;
    jsonData[String("c" + String(i) + "V")] = liveData->params.cellVoltage[i];
  }

  jsonData["debug"] = liveData->params.debugData;
  jsonData["debug2"] = liveData->params.debugData2;

  serializeJson(jsonData, Serial);
  serializeJson(jsonData, file);

  return true;
}

/**
 * Show Network Settings
 */
void BoardInterface::showNet()
{
  String prefix = "", suffix = "";

  syslog->print("wifiSsid:  ");
  syslog->println(liveData->settings.wifiSsid);

  if (liveData->settings.backupWifiEnabled == 1)
  {
    syslog->println("Backup wifi exists");
    syslog->print("wifiSsid2: ");
    syslog->println(liveData->settings.wifiSsid2);
  }
  else
  {
    syslog->println("No wifi backup configured");
  }
  if (liveData->params.isWifiBackupLive == true)
  {
    suffix = "backup";
  }
  else
  {
    suffix = "main";
  }

  syslog->print("Active: ");
  syslog->println(suffix);
  syslog->print("IP-Address: ");
  syslog->println(WiFi.localIP().toString());
}

/**
 * Show time
 */
void BoardInterface::showTime()
{
  struct tm now;
  getLocalTime(&now);
  char dts[32];
  strftime(dts, sizeof(dts), "%Y-%m-%d %X", &now);
  syslog->print("Current time: ");
  syslog->println(dts);
}

/**
 * Set time
 */
void BoardInterface::setTime(String timestamp)
{
  struct timeval tv;
  struct tm tm_tmp;
  tm_tmp.tm_year = timestamp.substring(0, 4).toInt() - 1900;
  tm_tmp.tm_mon = timestamp.substring(5, 7).toInt() - 1;
  tm_tmp.tm_mday = timestamp.substring(8, 10).toInt();
  tm_tmp.tm_hour = timestamp.substring(11, 13).toInt();
  tm_tmp.tm_min = timestamp.substring(14, 16).toInt();
  tm_tmp.tm_sec = timestamp.substring(17, 19).toInt();

  time_t t = mktime(&tm_tmp);
  tv.tv_sec = t;

  settimeofday(&tv, NULL);
  struct tm tm;
  getLocalTime(&tm);
  liveData->params.currentTime = mktime(&tm);
  liveData->params.chargingStartTime = liveData->params.currentTime;

  syslog->println("New time set. Only M5 Core2 is supported.");
  showTime();
}

/**
 * Automatic brightness by sunset/sunrise
 */
void BoardInterface::calcAutomaticBrightnessLatLon()
{
  if (liveData->settings.lcdBrightness == 0) // only for automatic mode
  {
    if (liveData->params.lcdBrightnessCalc == -1 && liveData->params.gpsLat != -1.0 && liveData->params.gpsLon != -1.0)
    {
      initSolarCalc(liveData->settings.timezone, liveData->params.gpsLat, liveData->params.gpsLon);
    }
    // angle from zenith
    // <70 = 100% brightnesss
    // >100 = 10%
    double sunDeg = getSZA(liveData->params.currentTime);
    syslog->infoNolf(DEBUG_GPS, "SUN from zenith, degrees: ");
    syslog->info(DEBUG_GPS, sunDeg);
    int32_t newBrightness = (105 - sunDeg) * 3.5;
    newBrightness = (newBrightness < 5 ? 5 : (newBrightness > 100) ? 100
                                                                   : newBrightness);
    if (liveData->params.lcdBrightnessCalc != newBrightness)
    {
      liveData->params.lcdBrightnessCalc = newBrightness;
      syslog->print("New automatic brightness: ");
      syslog->println(newBrightness);
      setBrightness();
    }
  }
}
