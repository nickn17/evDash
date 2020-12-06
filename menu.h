

#include "config.h";

MENU_ITEM menuItemsSource[100] = {

  {0, 0, 0, "<- exit menu"},
  {MENU_VEHICLE_TYPE, 0, -1, "Vehicle type"},
  {2, 0, -1, "Select OBD2 BLE4 adapter"},
  {3, 0, -1, "Others"},
  {4, 0, -1, "Units"},
  {8, 0, -1, "Factory reset"},
  {MENU_SAVE_SETTINGS, 0, -1, "Save settings"},
  {MENU_APP_VERSION, 0, -1, "Version"},
  {11, 0, -1, "Shutdown"},

  {100, 1, 0, "<- parent menu"},
  {101, 1, -1,  "Kia eNiro 2020 64kWh"},
  {102, 1, -1,  "Hyundai Kona 2020 64kWh"},
  {103, 1, -1,  "Hyundai Ioniq 2018 28kWh"},
  {104, 1, -1,  "Kia eNiro 2020 39kWh"},
  {105, 1, -1,  "Hyundai Kona 2020 39kWh"},
  {106, 1, -1,  "Renault Zoe 22kWh (DEV)"},
  {107, 1, -1,  "Debug OBD2 Kia"},

  {300, 3, 0, "<- parent menu"},
  // {MENU_WIFI, 3, -1, "[dev] WiFi network"},
  {MENU_GPRS, 3, -1, "[dev] GSM/GPRS"},
  {MENU_REMOTE_UPLOAD, 3, -1, "[dev] Remote upload"},
  {MENU_NTP, 3, -1, "[dev] NTP"},
  {MENU_SDCARD, 3, -1, "SD card"},
  {MENU_SCREEN_ROTATION, 3, -1, "Screen rotation"},
  {MENU_DEFAULT_SCREEN, 3, -1, "Default screen"},
  {MENU_SCREEN_BRIGHTNESS, 3, -1, "LCD brightness"},
  {MENU_PREDRAWN_GRAPHS, 3, -1, "Pre-drawn ch.graphs"},
  {MENU_HEADLIGHTS_REMINDER, 3, -1, "Headlight reminder"},
  {MENU_DEBUG_SCREEN, 3, -1, "Debug screen"},
  
/*
  // NTP
  byte ntpEnabled; // 0/1 
  byte ntpTimezone;
  byte ntpDaySaveTime; // 0/1
  // GPRS SIM800L 
  byte gprsEnabled; // 0/1 
  char gprsApn[64];
  // Remote upload
  byte remoteUploadEnabled; // 0/1 
  char remoteApiUrl[64];
  char remoteApiKey[32];*/

  {400, 4, 0, "<- parent menu"},
  {MENU_DISTANCE_UNIT, 4, -1, "Distance"},
  {MENU_TEMPERATURE_UNIT, 4, -1, "Temperature"},
  {MENU_PRESSURE_UNIT, 4, -1, "Pressure"},

  {3010, 301, 3, "<- parent menu"},
  {MENU_WIFI_ENABLED, 301, -1, "WiFi enabled"},
  {MENU_WIFI_SSID, 301, -1, "SSID"},
  {MENU_WIFI_PASSWORD, 301, -1, "Password"},

  {3040, 304, 3, "<- parent menu"},
  {MENU_SDCARD_ENABLED, 304, -1, "SD enabled"},
  {MENU_SDCARD_AUTOSTARTLOG, 304, -1, "Autostart log enabled"},
  {MENU_SDCARD_MOUNT_STATUS, 304, -1, "Status"},
  {MENU_SDCARD_REC, 304, -1, "Record"},
  
  {3070, 307, 3, "<- parent menu"},
  {3071, 307, -1, "Auto mode"},
  {3072, 307, -1, "Basic info"},
  {3073, 307, -1, "Speed"},
  {3074, 307, -1, "Battery cells"},
  {3075, 307, -1, "Charging graph"},

  {4010, 401, 4, "<- parent menu"},
  {4011, 401, -1, "Kilometers"},
  {4012, 401, -1, "Miles"},

  {4020, 402, 4, "<- parent menu"},
  {4021, 402, -1, "Celsius"},
  {4022, 402, -1, "Fahrenheit"},

  {4030, 403, 4, "<- parent menu"},
  {4031, 403, -1, "Bar"},
  {4032, 403, -1, "Psi"},

  {9999, 9998, 0, "List of BLE devices"},
  {10000, 9999, 0, "<- parent menu"},
  {10001, 9999, -1, "-"},
  {10002, 9999, -1, "-"},
  {10003, 9999, -1, "-"},
  {10004, 9999, -1, "-"},
  {10005, 9999, -1, "-"},
  {10006, 9999, -1, "-"},
  {10007, 9999, -1, "-"},
  {10008, 9999, -1, "-"},
  {10009, 9999, -1, "-"},
};
