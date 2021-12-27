#pragma once

#include "config.h"

#define MENU_SIZE 113


MENU_ITEM menuItemsSource[MENU_SIZE] = {
//   menu_id,                       parent_menu,        target_menu,    menu_str
    {MENU_TOP_LEVEL,                MENU_TOP_LEVEL,     MENU_TOP_LEVEL,   "<- exit menu"},
    {MENU_VEHICLE_TYPE,             MENU_TOP_LEVEL,     MENU_NO_MENU,     "Vehicle type"},
    {MENU_ADAPTER_TYPE,             MENU_TOP_LEVEL,     MENU_NO_MENU,     "Adapter type"},
    {MENU_ADAPTER_BLE_SELECT,       MENU_TOP_LEVEL,     MENU_NO_MENU,     "Select OBD2 BLE4 adapter"},
    {MENU_OTHERS,                   MENU_TOP_LEVEL,     MENU_NO_MENU,     "Others"},
    {MENU_UNITS,                    MENU_TOP_LEVEL,     MENU_NO_MENU,     "Units"},
    {MENU_FACTORY_RESET,            MENU_TOP_LEVEL,     MENU_NO_MENU,     "Factory reset"},
    {MENU_SAVE_SETTINGS,            MENU_TOP_LEVEL,     MENU_NO_MENU,     "Save settings"},
    {MENU_APP_VERSION,              MENU_TOP_LEVEL,     MENU_NO_MENU,     "OTA update, curr:"},
    {MENU_SHUTDOWN,                 MENU_TOP_LEVEL,     MENU_NO_MENU,     "Shutdown"},
    {MENU_CAR_COMMANDS,             MENU_TOP_LEVEL,     MENU_NO_MENU,     "Car commands [CAN]"},

    {VEHICLE_TYPE_TOP,              MENU_VEHICLE_TYPE,  MENU_TOP_LEVEL,   "<- parent menu"},
    {VEHICLE_TYPE_ENIRO_2020_64,    MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Kia eNiro 2020 64kWh"},
    {VEHICLE_TYPE_ENIRO_2020_39,    MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Kia eNiro 2020 39kWh"},
    {VEHICLE_TYPE_ESOUL_2020_64,    MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Kia eSoul 2020 64kWh"},
    {VEHICLE_TYPE_IONIQ_2018_28,    MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Hyundai Ioniq 2018 28kWh"},
    {VEHICLE_TYPE_KONA_2020_64,     MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Hyundai Kona 2020 64kWh"},
    {VEHICLE_TYPE_KONA_2020_39,     MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Hyundai Kona 2020 39kWh"},
    {VEHICLE_TYPE_HYUNDAI_IONIQ5_58,  MENU_VEHICLE_TYPE,  MENU_NO_MENU,   "Hyundai Ioniq5 2022 58kWh"},
    {VEHICLE_TYPE_HYUNDAI_IONIQ5_72,  MENU_VEHICLE_TYPE,  MENU_NO_MENU,   "Hyundai Ioniq5 2022 72.6kWh"},
    {VEHICLE_TYPE_VW_ID3_2021_45,   MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "VW ID.3 2021 45 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID3_2021_58,   MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "VW ID.3 2021 58 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID3_2021_77,   MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "VW ID.3 2021 77 kWh (CAN)"},
    {VEHICLE_TYPE_ZOE_22_DEV,       MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Renault Zoe 22kWh (DEV)"},
    {VEHICLE_TYPE_BMWI3_2014_22,    MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "BMW i3 2014 22kWh (DEV)"},
    {VEHICLE_TYPE_PEUGEOT_E208,     MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Peugeot e-208 (DEV)"},
    {VEHICLE_TYPE_DEBUG_OBD_KIA,    MENU_VEHICLE_TYPE,  MENU_NO_MENU,     "Debug OBD2 Kia"},

    {MENU_ADAPTER_TYPE_TOP,         MENU_ADAPTER_TYPE,  MENU_TOP_LEVEL,   "<- parent menu"},
    {MENU_ADAPTER_BLE4,             MENU_ADAPTER_TYPE,  MENU_NO_MENU,     "Bluetooth4 (BLE4)"},
    {MENU_ADAPTER_CAN,              MENU_ADAPTER_TYPE,  MENU_NO_MENU,     "CAN bus (MCP2515-1/SO)"},
    {MENU_ADAPTER_BT3,              MENU_ADAPTER_TYPE,  MENU_NO_MENU,     "Bluetooth3 serial"},
    {MENU_ADAPTER_LOAD_TEST_DATA,              MENU_ADAPTER_TYPE,  MENU_NO_MENU,     "Load demo data"},

    {FACTORY_RESET_TOP,             MENU_FACTORY_RESET, MENU_TOP_LEVEL,   "<- parent menu"},
    {FACTORY_RESET_CONFIRM,         MENU_FACTORY_RESET, MENU_NO_MENU,     "Confirm factory reset"},

    {MENU_OTHER_TOP,                MENU_OTHERS,        MENU_TOP_LEVEL,   "<- parent menu"},
    {MENU_WIFI,                     MENU_OTHERS,        MENU_NO_MENU,     "WiFi network"},
    {MENU_SDCARD,                   MENU_OTHERS,        MENU_NO_MENU,     "SD card"},
    {MENU_GPS,                      MENU_OTHERS,        MENU_NO_MENU,     "GPS"},
    {MENU_REMOTE_UPLOAD,            MENU_OTHERS,        MENU_NO_MENU,     "Rem. Upload"},
    {MENU_SERIAL_CONSOLE,           MENU_OTHERS,        MENU_NO_MENU,     "Serial console"},
    {MENU_VOLTMETER,                MENU_OTHERS,        MENU_NO_MENU,     "Voltmeter INA3221"},
    {MENU_DEBUG_LEVEL,              MENU_OTHERS,        MENU_NO_MENU,     "Debug level"},
    {MENU_SCREEN_ROTATION,          MENU_OTHERS,        MENU_NO_MENU,     "Screen rotation"},
    {MENU_DEFAULT_SCREEN,           MENU_OTHERS,        MENU_NO_MENU,     "Default screen"},
    {MENU_SCREEN_BRIGHTNESS,        MENU_OTHERS,        MENU_NO_MENU,     "LCD brightness"},
    {MENU_PREDRAWN_GRAPHS,          MENU_OTHERS,        MENU_NO_MENU,     "Pre-drawn ch.graphs"},
    {MENU_HEADLIGHTS_REMINDER,      MENU_OTHERS,        MENU_NO_MENU,     "Headlight reminder"},
    {MENU_SLEEP_MODE,               MENU_OTHERS,        MENU_NO_MENU,     "SleepMode"},
    {MENU_CURRENT_TIME,             MENU_OTHERS,        MENU_NO_MENU,     "Time:"},
    {MENU_TIMEZONE,                 MENU_OTHERS,        MENU_NO_MENU,     "Timezone"},
    {MENU_DAYLIGHT_SAVING,          MENU_OTHERS,        MENU_NO_MENU,     "Daylight saving"},
    {MENU_RHD,                      MENU_OTHERS,        MENU_NO_MENU,     "RHD (Right hand drive)"},

    {MENU_UNIT_TOP,                 MENU_UNITS,         MENU_TOP_LEVEL,   "<- parent menu"},
    {MENU_DISTANCE_UNIT,            MENU_UNITS,         MENU_NO_MENU,     "Distance"},
    {MENU_TEMPERATURE_UNIT,         MENU_UNITS,         MENU_NO_MENU,     "Temperature"},
    {MENU_PRESSURE_UNIT,            MENU_UNITS,         MENU_NO_MENU,     "Pressure"},

    {MENU_WIFI_TOP,                 MENU_WIFI,          MENU_OTHERS,      "<- parent menu"},
    {MENU_WIFI_ENABLED,             MENU_WIFI,          MENU_NO_MENU,     "WiFi enabled"},
    {MENU_WIFI_SSID,                MENU_WIFI,          MENU_NO_MENU,     "SSID:"},
    {MENU_WIFI_PASSWORD,            MENU_WIFI,          MENU_NO_MENU,     "Password:"},
    {MENU_WIFI_SSID2,               MENU_WIFI,          MENU_NO_MENU,     "SSID2:"},
    {MENU_WIFI_PASSWORD2,           MENU_WIFI,          MENU_NO_MENU,     "Password2:"},
    {MENU_WIFI_NTP,                 MENU_WIFI,          MENU_NO_MENU,     "NTP sync:"},
    {MENU_WIFI_ACTIVE,              MENU_WIFI,          MENU_NO_MENU,     "Active:"},
    {MENU_WIFI_IPADDR,              MENU_WIFI,          MENU_NO_MENU,     "IP addr:"},

    {MENU_SDCARD_TOP,               MENU_SDCARD,        MENU_OTHERS,      "<- parent menu"},
    {MENU_SDCARD_ENABLED,           MENU_SDCARD,        MENU_NO_MENU,     "SD enabled"},
    {MENU_SDCARD_AUTOSTARTLOG,      MENU_SDCARD,        MENU_NO_MENU,     "Autostart log enabled"},
    {MENU_SDCARD_MOUNT_STATUS,      MENU_SDCARD,        MENU_NO_MENU,     "Status"},
    {MENU_SDCARD_REC,               MENU_SDCARD,        MENU_NO_MENU,     "Record"},
//  {MENU_SDCARD_INTERVAL,          MENU_SDCARD,        MENU_NO_MENU,     "Log interval sec."},

    {MENU_VOLTMETER_TOP,            MENU_VOLTMETER,     MENU_OTHERS,      "<- parent menu"},
    {MENU_VOLTMETER_ENABLED,        MENU_VOLTMETER,     MENU_NO_MENU,     "Voltmeter enabled"},
    {MENU_VOLTMETER_SLEEP,          MENU_VOLTMETER,     MENU_NO_MENU,     "Control SleepMode"},
    {MENU_VOLTMETER_SLEEPVOL,       MENU_VOLTMETER,     MENU_NO_MENU,     "Sleep Vol."},
    {MENU_VOLTMETER_WAKEUPVOL,      MENU_VOLTMETER,     MENU_NO_MENU,     "WakeUp Vol."},
    {MENU_VOLTMETER_CUTOFFVOL,      MENU_VOLTMETER,     MENU_NO_MENU,     "CutOff Vol."},

    {MENU_SLEEP_TOP,                MENU_SLEEP_MODE,    MENU_OTHERS,      "<- parent menu"},
    {MENU_SLEEP_MODE_MODE,          MENU_SLEEP_MODE,    MENU_NO_MENU,     "Mode"},
    {MENU_SLEEP_MODE_WAKEINTERVAL,  MENU_SLEEP_MODE,    MENU_NO_MENU,     "WakeUp Check"},
    {MENU_SLEEP_MODE_SHUTDOWNHRS,   MENU_SLEEP_MODE,    MENU_NO_MENU,     "Shutdown After"},

    {MENU_REMOTE_UPLOAD_TOP,        MENU_REMOTE_UPLOAD, MENU_OTHERS,      "<- parent menu"},
    {MENU_REMOTE_UPLOAD_UART,       MENU_REMOTE_UPLOAD, MENU_NO_MENU,     "SerialConsole"},
    {MENU_REMOTE_UPLOAD_TYPE,       MENU_REMOTE_UPLOAD, MENU_NO_MENU,     "Module Type"},
    {MENU_REMOTE_UPLOAD_API_INTERVAL,  MENU_REMOTE_UPLOAD, MENU_NO_MENU,  "API Upload Int."},
    {MENU_REMOTE_UPLOAD_ABRP_INTERVAL, MENU_REMOTE_UPLOAD, MENU_NO_MENU,  "ABRP Upload Int."},

    {DEFAULT_SCREEN_TOP,            MENU_DEFAULT_SCREEN, MENU_OTHERS,     "<- parent menu"},
    {DEFAULT_SCREEN_AUTOMODE,       MENU_DEFAULT_SCREEN, MENU_NO_MENU,    "Auto mode"},
    {DEFAULT_SCREEN_BASIC_INFO,     MENU_DEFAULT_SCREEN, MENU_NO_MENU,    "Basic info"},
    {DEFAULT_SCREEN_SPEED,          MENU_DEFAULT_SCREEN, MENU_NO_MENU,    "Speed"},
    {DEFAULT_SCREEN_BATT_CELL,      MENU_DEFAULT_SCREEN, MENU_NO_MENU,    "Battery cells"},
    {DEFAULT_SCREEN_CHG_GRAPH,      MENU_DEFAULT_SCREEN, MENU_NO_MENU,    "Charging graph"},
    {DEFAULT_SCREEN_HUD,            MENU_DEFAULT_SCREEN, MENU_NO_MENU,    "HUD"},

    {DISTANCE_UNIT_TOP,             MENU_DISTANCE_UNIT,  MENU_UNITS,     "<- parent menu"},
    {DISTANCE_UNIT_KM,              MENU_DISTANCE_UNIT,  MENU_NO_MENU,   "Kilometers"},
    {DISTANCE_UNIT_MI,              MENU_DISTANCE_UNIT,  MENU_NO_MENU,   "Miles"},



    {TEMPERATURE_UNIT_TOP,       MENU_TEMPERATURE_UNIT,  MENU_UNITS,     "<- parent menu"},
    {TEMPERATURE_UNIT_CEL,       MENU_TEMPERATURE_UNIT,  MENU_NO_MENU,   "Celsius"},
    {TEMPERATURE_UNIT_FAR,       MENU_TEMPERATURE_UNIT,  MENU_NO_MENU,   "Fahrenheit"},

    {PRESURE_UNIT_TOP,              MENU_PRESSURE_UNIT,  MENU_UNITS,     "<- parent menu"},
    {PRESURE_UNIT_BAR,              MENU_PRESSURE_UNIT,  MENU_NO_MENU,   "Bar"},
    {PRESURE_UNIT_BAR,              MENU_PRESSURE_UNIT,  MENU_NO_MENU,   "Psi"},

    {LIST_OF_BLE_DEV_TOP,           LIST_OF_BLE_DEV,     MENU_TOP_LEVEL, "List of BLE devices"},
    {LIST_OF_BLE_1,                 LIST_OF_BLE_DEV,     MENU_TOP_LEVEL, "<- parent menu"},
    {LIST_OF_BLE_2,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_3,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_4,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_5,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_6,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_7,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_8,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_9,                 LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},
    {LIST_OF_BLE_10,                LIST_OF_BLE_DEV,     MENU_NO_MENU,   "-"},

    {MENU_CAR_COMMANDS_TOP,       MENU_CAR_COMMANDS,     MENU_TOP_LEVEL, "<- parent menu"}
};
