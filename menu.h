// Menu id/parent/title
typedef struct {
  int16_t id;
  int16_t parentId;
  int16_t targetParentId;
  char title[50];
  char obdMacAddress[20];
  char serviceUUID[40];
} MENU_ITEM;

#define menuItemsCount 66
bool menuVisible = false;
uint16_t menuCurrent = 0;
uint8_t  menuItemSelected = 0;
uint16_t scanningDeviceIndex = 0;
MENU_ITEM menuItems[menuItemsCount] = {

  {0, 0, 0, "<- exit menu"},
  {1, 0, -1, "Vehicle type"},
  {2, 0, -1, "Select OBD2BLE adapter"},
  {3, 0, -1, "Others"},
  {4, 0, -1, "Units"},
  {8, 0, -1, "Factory reset"},
  {9, 0, -1, "Save settings"},
  {10, 0, -1, "Version"},

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
  {302, 3, -1, "Default screen"},
  {303, 3, -1, "Debug screen off/on"},
  {304, 3, -1, "LCD brightness"},
  {305, 3, -1, "Pre-drawn ch.graphs 0/1"},

  {400, 4, 0, "<- parent menu"},
  {401, 4, -1, "Distance"},
  {402, 4, -1, "Temperature"},
  {403, 4, -1, "Pressure"},

  {3010, 301, 3, "<- parent menu"},
  {3011, 301, -1, "Normal"},
  {3012, 301, -1, "Flip vertical"},

  {3020, 302, 3, "<- parent menu"},
  {3021, 302, -1, "Auto mode"},
  {3022, 302, -1, "Basic info"},
  {3023, 302, -1, "Speed"},
  {3024, 302, -1, "Battery cells"},
  {3025, 302, -1, "Charging graph"},

  {3030, 303, 3, "<- parent menu"},
  {3031, 303, -1, "Off"},
  {3032, 303, -1, "On"},

  {3040, 304, 3, "<- parent menu"},
  {3041, 304, -1, "Auto"},
  {3042, 304, -1, "20%"},
  {3043, 304, -1, "50%"},
  {3044, 304, -1, "100%"},

  {3050, 305, 3, "<- parent menu"},
  {3051, 305, -1, "Off"},
  {3052, 305, -1, "On"},

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
