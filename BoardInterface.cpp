#ifndef BOARDINTERFACE_CPP
#define BOARDINTERFACE_CPP

#include <EEPROM.h>
#include <BLEDevice.h>
#include "BoardInterface.h"
#include "LiveData.h"

/**
   Set live data
*/
void BoardInterface::setLiveData(LiveData* pLiveData) {
  this->liveData = pLiveData;
}

/**
   Attach car interface
*/
void BoardInterface::attachCar(CarInterface* pCarInterface) {
  this->carInterface = pCarInterface;
}


/**
  Shutdown device
*/
void BoardInterface::shutdownDevice() {

  Serial.println("Shutdown.");

  this->displayMessage("Shutdown in 3 sec.", "");
  delay(3000);

  setCpuFrequencyMhz(80);
  this->setBrightness(0);
  //WiFi.disconnect(true);
  //WiFi.mode(WIFI_OFF);
  btStop();
  //adc_power_off();
  //esp_wifi_stop();
  esp_bt_controller_disable();

  delay(2000);
  //esp_sleep_enable_timer_wakeup(525600L * 60L * 1000000L); // minutes
  esp_deep_sleep_start();
}

/**
  Load settings from flash memory, upgrade structure if version differs
*/
void BoardInterface::saveSettings() {

  // Flash to memory
  Serial.println("Settings saved to eeprom.");
  EEPROM.put(0, this->liveData->settings);
  EEPROM.commit();
}

/**
  Reset settings (factory reset)
*/
void BoardInterface::resetSettings() {

  // Flash to memory
  Serial.println("Factory reset.");
  this->liveData->settings.initFlag = 1;
  EEPROM.put(0, this->liveData->settings);
  EEPROM.commit();

  this->displayMessage("Settings erased", "Restarting in 5 seconds");

  delay(5000);
  ESP.restart();
}

/**
  Load setting from flash memory, upgrade structure if version differs
*/
void BoardInterface::loadSettings() {

  String tmpStr;

  // Init
  this->liveData->settings.initFlag = 183;
  this->liveData->settings.settingsVersion = 3;
  this->liveData->settings.carType = CAR_KIA_ENIRO_2020_64;

  // Default OBD adapter MAC and UUID's
  tmpStr = "00:00:00:00:00:00"; // Pair via menu (middle button)
  tmpStr.toCharArray(this->liveData->settings.obdMacAddress, tmpStr.length() + 1);
  tmpStr = "000018f0-0000-1000-8000-00805f9b34fb"; // Default UUID's for VGate iCar Pro BLE4 adapter
  tmpStr.toCharArray(this->liveData->settings.serviceUUID, tmpStr.length() + 1);
  tmpStr = "00002af0-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(this->liveData->settings.charTxUUID, tmpStr.length() + 1);
  tmpStr = "00002af1-0000-1000-8000-00805f9b34fb";
  tmpStr.toCharArray(this->liveData->settings.charRxUUID, tmpStr.length() + 1);

  this->liveData->settings.displayRotation = 1; // 1,3
  this->liveData->settings.distanceUnit = 'k';
  this->liveData->settings.temperatureUnit = 'c';
  this->liveData->settings.pressureUnit = 'b';
  this->liveData->settings.defaultScreen = 1;
  this->liveData->settings.lcdBrightness = 0;
  this->liveData->settings.debugScreen = 0;
  this->liveData->settings.predrawnChargingGraphs = 1;

#ifdef SIM800L_ENABLED
  tmpStr = "internet.t-mobile.cz";
  tmpStr.toCharArray(liveData->settings.gprsApn, tmpStr.length() + 1);
  tmpStr = "http://api.example.com";
  tmpStr.toCharArray(liveData->settings.remoteApiSrvr, tmpStr.length() + 1);
  tmpStr = "example";
  tmpStr.toCharArray(liveData->settings.remoteApiKey, tmpStr.length() + 1);
#endif //SIM800L_ENABLED

  // Load settings and replace default values
  Serial.println("Reading settings from eeprom.");
  EEPROM.begin(sizeof(SETTINGS_STRUC));
  EEPROM.get(0, this->liveData->tmpSettings);

  // Init flash with default settings
  if (this->liveData->tmpSettings.initFlag != 183) {
    Serial.println("Settings not found. Initialization.");
    this->saveSettings();
  } else {
    Serial.print("Loaded settings ver.: ");
    Serial.println(this->liveData->settings.settingsVersion);

    // Upgrade structure
    if (this->liveData->settings.settingsVersion != this->liveData->tmpSettings.settingsVersion) {
      if (this->liveData->tmpSettings.settingsVersion == 1) {
        this->liveData->tmpSettings.settingsVersion = 2;
        this->liveData->tmpSettings.defaultScreen = this->liveData->settings.defaultScreen;
        this->liveData->tmpSettings.lcdBrightness = this->liveData->settings.lcdBrightness;
        this->liveData->tmpSettings.debugScreen = this->liveData->settings.debugScreen;
      }
      if (this->liveData->tmpSettings.settingsVersion == 2) {
        this->liveData->tmpSettings.settingsVersion = 3;
        this->liveData->tmpSettings.predrawnChargingGraphs = this->liveData->settings.predrawnChargingGraphs;
      }
      this->saveSettings();
    }

    // Save version? No need to upgrade structure
    if (this->liveData->settings.settingsVersion == this->liveData->tmpSettings.settingsVersion) {
      this->liveData->settings = this->liveData->tmpSettings;
    }
  }
}

#endif // BOARDINTERFACE_CPP
