

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
  {301, 3, -1, "Screen rotation"},
  {MENU_DEFAULT_SCREEN, 3, -1, "Default screen"},
  {MENU_DEBUG_SCREEN, 3, -1, "Debug screen"},
  {MENU_SCREEN_BRIGHTNESS, 3, -1, "LCD brightness"},
  {MENU_PREDRAWN_GRAPHS, 3, -1, "Pre-drawn ch.graphs"},
  {MENU_WIFI, 3, -1, "WiFi network"},
  {307, 3, -1, "[DEV] SD card"},
  {MENU_GPRS, 3, -1, "GPRS"},
  {309, 3, -1, "[DEV] Remote upload"},
  {MENU_HEADLIGHTS_REMINDER, 3, -1, "Headlight reminder"},
  
/*
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
  char remoteApiKey[32];*/

  {400, 4, 0, "<- parent menu"},
  {MENU_DISTANCE_UNIT, 4, -1, "Distance"},
  {MENU_TEMPERATURE_UNIT, 4, -1, "Temperature"},
  {MENU_PRESSURE_UNIT, 4, -1, "Pressure"},

  {3010, 301, 3, "<- parent menu"},
  {3011, 301, -1, "Normal"},
  {3012, 301, -1, "Flip vertical"},

  {3020, 302, 3, "<- parent menu"},
  {3021, 302, -1, "Auto mode"},
  {3022, 302, -1, "Basic info"},
  {3023, 302, -1, "Speed"},
  {3024, 302, -1, "Battery cells"},
  {3025, 302, -1, "Charging graph"},

  {3040, 304, 3, "<- parent menu"},
  {3041, 304, -1, "Auto"},
  {3042, 304, -1, "20%"},
  {3043, 304, -1, "50%"},
  {3044, 304, -1, "100%"},

  {3060, 306, 3, "<- parent menu"},
  {MENU_WIFI_ENABLED, 306, -1, "WiFi enabled"},
  {MENU_WIFI_SSID, 306, -1, "SSID"},
  {MENU_WIFI_PASSWORD, 306, -1, "Password"},

  {3070, 307, 3, "<- parent menu"},
  {3071, 307, -1, "Info:"},
  {3072, 307, -1, "Mount manually"},
  {3073, 307, -1, "Record now"},
  {3074, 307, -1, "Stop recording"},
  {3075, 307, -1, "Record on boot off/on"},

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
