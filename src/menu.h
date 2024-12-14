#pragma once

#include "config.h"

#define MENU_SIZE 168

MENU_ITEM menuItemsSource[MENU_SIZE] = {
    //   menu_id,                       parent_menu,        target_menu,    menu_str
    {MENU_TOP_LEVEL, MENU_TOP_LEVEL, MENU_TOP_LEVEL, "<- exit menu"},
    {MENU_CLEAR_STATS, MENU_TOP_LEVEL, MENU_NO_MENU, "Clear stats"},
    {MENU_VEHICLE_TYPE, MENU_TOP_LEVEL, MENU_NO_MENU, "Vehicle type"},
    {MENU_ADAPTER_TYPE, MENU_TOP_LEVEL, MENU_NO_MENU, "Adapter (CAN/OBD2)"},
    {MENU_BOARD, MENU_TOP_LEVEL, MENU_NO_MENU, "Board setup"},
    {MENU_OTHERS, MENU_TOP_LEVEL, MENU_NO_MENU, "Others"},
    {MENU_UNITS, MENU_TOP_LEVEL, MENU_NO_MENU, "Units"},
    {MENU_FACTORY_RESET, MENU_TOP_LEVEL, MENU_NO_MENU, "Factory reset"},
    {MENU_SAVE_SETTINGS, MENU_TOP_LEVEL, MENU_NO_MENU, "Save settings"},
    {MENU_APP_VERSION, MENU_TOP_LEVEL, MENU_NO_MENU, "OTA update, curr:"},
    {MENU_MEMORY_USAGE, MENU_TOP_LEVEL, MENU_NO_MENU, "Memory usage"},
    {MENU_SHUTDOWN, MENU_TOP_LEVEL, MENU_NO_MENU, "Shutdown"},
    {MENU_REBOOT, MENU_TOP_LEVEL, MENU_NO_MENU, "Reboot"},
    {MENU_CAR_COMMANDS, MENU_TOP_LEVEL, MENU_NO_MENU, "Car commands [CAN]"},

    {VEHICLE_TYPE_TOP, MENU_VEHICLE_TYPE, MENU_TOP_LEVEL, "<- parent menu"},
    {VEHICLE_TYPE_HYUNDAI, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Hyundai"},
    {VEHICLE_TYPE_KIA, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Kia"},
    {VEHICLE_TYPE_AUDI, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Audi"},
    {VEHICLE_TYPE_SKODA, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Skoda"},
    {VEHICLE_TYPE_VW, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Volkswagen"},
    {VEHICLE_TYPE_ZOE_22_DEV, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Renault Zoe 22kWh (DEV)"},
    {VEHICLE_TYPE_BMWI3_2014_22, MENU_VEHICLE_TYPE, MENU_NO_MENU, "BMW i3 2014 22kWh (DEV)"},
    {VEHICLE_TYPE_PEUGEOT_E208, MENU_VEHICLE_TYPE, MENU_NO_MENU, "Peugeot e-208 (DEV)"},

    {VEHICLE_TYPE_HYUNDAI_TOP, VEHICLE_TYPE_HYUNDAI, MENU_TOP_LEVEL, "<- parent menu"},
    {VEHICLE_TYPE_IONIQ_2018_28, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Ioniq 2018 28kWh"},
    {VEHICLE_TYPE_IONIQ_2018_PHEV, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Ioniq 18 PHEV 8.8kWh"},
    {VEHICLE_TYPE_KONA_2020_64, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Kona 2020 64kWh"},
    {VEHICLE_TYPE_KONA_2020_39, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Kona 2020 39kWh"},
    {VEHICLE_TYPE_HYUNDAI_IONIQ5_58, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Ioniq5 2022 58kWh"},
    {VEHICLE_TYPE_HYUNDAI_IONIQ5_72, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Ioniq5 2022 72.6kWh"},
    {VEHICLE_TYPE_HYUNDAI_IONIQ5_77, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Ioniq5 2022 77.4kWh"},
    {VEHICLE_TYPE_HYUNDAI_IONIQ6_77, VEHICLE_TYPE_HYUNDAI, MENU_NO_MENU, "Hyundai Ioniq6 2023 77.4kWh"},

    {VEHICLE_TYPE_KIA_TOP, VEHICLE_TYPE_KIA, MENU_TOP_LEVEL, "<- parent menu"},
    {VEHICLE_TYPE_ENIRO_2020_64, VEHICLE_TYPE_KIA, MENU_NO_MENU, "Kia eNiro 2020 64kWh"},
    {VEHICLE_TYPE_ENIRO_2020_39, VEHICLE_TYPE_KIA, MENU_NO_MENU, "Kia eNiro 2020 39kWh"},
    {VEHICLE_TYPE_ESOUL_2020_64, VEHICLE_TYPE_KIA, MENU_NO_MENU, "Kia eSoul 2020 64kWh"},
    {VEHICLE_TYPE_KIA_EV6_58, VEHICLE_TYPE_KIA, MENU_NO_MENU, "Kia EV6 2022 58kWh"},
    {VEHICLE_TYPE_KIA_EV6_77, VEHICLE_TYPE_KIA, MENU_NO_MENU, "Kia EV6 2022 77.4kWh"},

    {VEHICLE_TYPE_AUDI_TOP, VEHICLE_TYPE_AUDI, MENU_TOP_LEVEL, "<- parent menu"},
    {VEHICLE_TYPE_AUDI_Q4_35, VEHICLE_TYPE_AUDI, MENU_NO_MENU, "Audi Q4 35 (CAN)"},
    {VEHICLE_TYPE_AUDI_Q4_40, VEHICLE_TYPE_AUDI, MENU_NO_MENU, "Audi Q4 40 (CAN)"},
    {VEHICLE_TYPE_AUDI_Q4_45, VEHICLE_TYPE_AUDI, MENU_NO_MENU, "Audi Q4 45 (CAN)"},
    {VEHICLE_TYPE_AUDI_Q4_50, VEHICLE_TYPE_AUDI, MENU_NO_MENU, "Audi Q4 50 (CAN)"},

    {VEHICLE_TYPE_SKODA_TOP, VEHICLE_TYPE_SKODA, MENU_TOP_LEVEL, "<- parent menu"},
    {VEHICLE_TYPE_SKODA_ENYAQ_55, VEHICLE_TYPE_SKODA, MENU_NO_MENU, "Skoda Enyaq iV 55 (CAN)"},
    {VEHICLE_TYPE_SKODA_ENYAQ_62, VEHICLE_TYPE_SKODA, MENU_NO_MENU, "Skoda Enyaq iV 62 (CAN)"},
    {VEHICLE_TYPE_SKODA_ENYAQ_82, VEHICLE_TYPE_SKODA, MENU_NO_MENU, "Skoda Enyaq iV 82 (CAN)"},

    {VEHICLE_TYPE_VW_TOP, VEHICLE_TYPE_VW, MENU_TOP_LEVEL, "<- parent menu"},
    {VEHICLE_TYPE_VW_ID3_2021_45, VEHICLE_TYPE_VW, MENU_NO_MENU, "VW ID.3 2021 45 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID3_2021_58, VEHICLE_TYPE_VW, MENU_NO_MENU, "VW ID.3 2021 58 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID3_2021_77, VEHICLE_TYPE_VW, MENU_NO_MENU, "VW ID.3 2021 77 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID4_2021_45, VEHICLE_TYPE_VW, MENU_NO_MENU, "VW ID.4 2021 45 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID4_2021_58, VEHICLE_TYPE_VW, MENU_NO_MENU, "VW ID.4 2021 58 kWh (CAN)"},
    {VEHICLE_TYPE_VW_ID4_2021_77, VEHICLE_TYPE_VW, MENU_NO_MENU, "VW ID.4 2021 77 kWh (CAN)"},

    {MENU_ADAPTER_TYPE_TOP, MENU_ADAPTER_TYPE, MENU_TOP_LEVEL, "<- parent menu"},
    {MENU_ADAPTER_BLE_SELECT, MENU_ADAPTER_TYPE, MENU_NO_MENU, "Select OBD2 adapter"},
    {MENU_ADAPTER_CAN_COMMU, MENU_ADAPTER_TYPE, MENU_NO_MENU, "CAN bus (MCP2515-1/SO)"},
    {MENU_ADAPTER_OBD2_BLE4, MENU_ADAPTER_TYPE, MENU_NO_MENU, "OBD2 Bluetooth4 (BLE4)"},
    {MENU_ADAPTER_OBD2_WIFI, MENU_ADAPTER_TYPE, MENU_NO_MENU, "OBD2 WIFI adapter [DEV]"},
    {MENU_ADAPTER_OBD2_NAME, MENU_ADAPTER_TYPE, MENU_NO_MENU, "[i] OBD2 name:"},
    {MENU_ADAPTER_OBD2_IP, MENU_ADAPTER_TYPE, MENU_NO_MENU, "[i] Wifi ip:"},
    {MENU_ADAPTER_OBD2_PORT, MENU_ADAPTER_TYPE, MENU_NO_MENU, "[i] Wifi port:"},
    {MENU_ADAPTER_COMMAND_QUEUE_AUTOSTOP, MENU_ADAPTER_TYPE, MENU_NO_MENU, "CAN queue autostop"},
    {MENU_ADAPTER_DISABLE_COMMAND_OPTIMIZER, MENU_ADAPTER_TYPE, MENU_NO_MENU, "Disable CMD optimizer"},
    {MENU_ADAPTER_LOAD_TEST_DATA, MENU_ADAPTER_TYPE, MENU_NO_MENU, "Load demo (static) data..."},

    {MENU_OTHER_TOP, MENU_OTHERS, MENU_TOP_LEVEL, "<- parent menu"},
    {MENU_WIFI, MENU_OTHERS, MENU_NO_MENU, "WiFi network"},
    {MENU_SDCARD, MENU_OTHERS, MENU_NO_MENU, "SD card"},
    {MENU_GPS, MENU_OTHERS, MENU_NO_MENU, "GPS"},
    {MENU_REMOTE_UPLOAD, MENU_OTHERS, MENU_NO_MENU, "Rem. Upload"},
    {MENU_VOLTMETER, MENU_OTHERS, MENU_NO_MENU, "Voltmeter INA3221"},
    {MENU_SPEED_CORRECTION, MENU_OTHERS, MENU_NO_MENU, "Speed correction:"},
    {MENU_RHD, MENU_OTHERS, MENU_NO_MENU, "RHD (Right hand drive)"},

    {MENU_BOARD_TOP, MENU_BOARD, MENU_TOP_LEVEL, "<- parent menu"},
    {MENU_BOARD_POWER_MODE, MENU_BOARD, MENU_NO_MENU, "Power mode:"},
    {MENU_CURRENT_TIME, MENU_BOARD, MENU_NO_MENU, "Time:"},
    {MENU_TIMEZONE, MENU_BOARD, MENU_NO_MENU, "Timezone"},
    {MENU_DAYLIGHT_SAVING, MENU_BOARD, MENU_NO_MENU, "Daylight saving"},
    {MENU_DEFAULT_SCREEN, MENU_BOARD, MENU_NO_MENU, "Default screen"},
    {MENU_SCREEN_ROTATION, MENU_BOARD, MENU_NO_MENU, "Screen rotation"},
    {MENU_SCREEN_BRIGHTNESS, MENU_BOARD, MENU_NO_MENU, "LCD brightness"},
    {MENU_SLEEP_MODE, MENU_BOARD, MENU_NO_MENU, "SleepMode"},
    {MENU_SERIAL_CONSOLE, MENU_BOARD, MENU_NO_MENU, "Serial console"},
    {MENU_DEBUG_LEVEL, MENU_BOARD, MENU_NO_MENU, "Debug level"},

    {MENU_UNIT_TOP, MENU_UNITS, MENU_TOP_LEVEL, "<- parent menu"},
    {MENU_DISTANCE_UNIT, MENU_UNITS, MENU_NO_MENU, "Distance"},
    {MENU_TEMPERATURE_UNIT, MENU_UNITS, MENU_NO_MENU, "Temperature"},
    {MENU_PRESSURE_UNIT, MENU_UNITS, MENU_NO_MENU, "Pressure"},

    {MENU_WIFI_TOP, MENU_WIFI, MENU_OTHERS, "<- parent menu"},
    {MENU_WIFI_ENABLED, MENU_WIFI, MENU_NO_MENU, "WiFi enabled"},
    {MENU_WIFI_HOTSPOT_WEBADMIN, MENU_WIFI, MENU_NO_MENU, "Enable hotspot+webadmin"},
    {MENU_WIFI_SSID, MENU_WIFI, MENU_NO_MENU, "[i] SSID:"},
    {MENU_WIFI_PASSWORD, MENU_WIFI, MENU_NO_MENU, "[i] Passwd:"},
    {MENU_WIFI_ENABLED2, MENU_WIFI, MENU_NO_MENU, "Backup WiFi enabled"},
    {MENU_WIFI_SSID2, MENU_WIFI, MENU_NO_MENU, "[i] SSID2:"},
    {MENU_WIFI_PASSWORD2, MENU_WIFI, MENU_NO_MENU, "[i] Passwd2:"},
    {MENU_WIFI_NTP, MENU_WIFI, MENU_NO_MENU, "NTP sync:"},
    {MENU_WIFI_ACTIVE, MENU_WIFI, MENU_NO_MENU, "[i] Active:"},
    {MENU_WIFI_IPADDR, MENU_WIFI, MENU_NO_MENU, "[i] IP addr:"},

    {MENU_SDCARD_TOP, MENU_SDCARD, MENU_OTHERS, "<- parent menu"},
    {MENU_SDCARD_ENABLED, MENU_SDCARD, MENU_NO_MENU, "SD enabled"},
    {MENU_SDCARD_AUTOSTARTLOG, MENU_SDCARD, MENU_NO_MENU, "Autostart log enabled"},
    {MENU_SDCARD_MOUNT_STATUS, MENU_SDCARD, MENU_NO_MENU, "Status"},
    {MENU_SDCARD_REC, MENU_SDCARD, MENU_NO_MENU, "Record"},
    {MENU_SDCARD_SETTINGS_SAVE, MENU_SDCARD, MENU_NO_MENU, "Backup settings to SDCARD"},
    {MENU_SDCARD_SETTINGS_RESTORE, MENU_SDCARD, MENU_NO_MENU, "Restore settings from SD"},
    {MENU_SDCARD_SAVE_CONSOLE_TO_SDCARD, MENU_SDCARD, MENU_NO_MENU, "Save console to SD card"},
    //  {MENU_SDCARD_INTERVAL,          MENU_SDCARD,        MENU_NO_MENU,     "Log interval sec."},

    {MENU_VOLTMETER_TOP, MENU_VOLTMETER, MENU_OTHERS, "<- parent menu"},
    {MENU_VOLTMETER_ENABLED, MENU_VOLTMETER, MENU_NO_MENU, "Voltmeter enabled"},
    {MENU_VOLTMETER_SLEEP, MENU_VOLTMETER, MENU_NO_MENU, "Control SleepMode"},
    {MENU_VOLTMETER_INFO, MENU_VOLTMETER, MENU_NO_MENU, "Show voltage/current"},
    {MENU_VOLTMETER_SLEEPVOL, MENU_VOLTMETER, MENU_NO_MENU, "Sleep Vol."},
    {MENU_VOLTMETER_WAKEUPVOL, MENU_VOLTMETER, MENU_NO_MENU, "WakeUp Vol."},
    {MENU_VOLTMETER_CUTOFFVOL, MENU_VOLTMETER, MENU_NO_MENU, "CutOff Vol."},

    {MENU_SLEEP_TOP, MENU_SLEEP_MODE, MENU_OTHERS, "<- parent menu"},
    {MENU_SLEEP_MODE_MODE, MENU_SLEEP_MODE, MENU_NO_MENU, "Mode"},
    // 202408: removed: {MENU_SLEEP_MODE_WAKEINTERVAL, MENU_SLEEP_MODE, MENU_NO_MENU, "WakeUp Check"},
    // 202408: removed: {MENU_SLEEP_MODE_SHUTDOWNHRS, MENU_SLEEP_MODE, MENU_NO_MENU, "Shutdown After"},

    {MENU_REMOTE_UPLOAD_TOP, MENU_REMOTE_UPLOAD, MENU_OTHERS, "<- parent menu"},
    {MENU_REMOTE_UPLOAD_UART, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "SerialConsole"},
    {MENU_REMOTE_UPLOAD_TYPE, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "Module Type"},
    {MENU_REMOTE_UPLOAD_API_INTERVAL, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "API Upload Int."},
    {MENU_REMOTE_UPLOAD_ABRP_INTERVAL, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "ABRP Upload Int."},
    {MENU_REMOTE_UPLOAD_ABRP_LOG_SDCARD, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "Log ABRP to SD card"},
    {MENU_REMOTE_UPLOAD_MQTT_ENABLED, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "Send MQTT data"},
    {MENU_REMOTE_UPLOAD_MQTT_SERVER, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "MQTT server"},
    {MENU_REMOTE_UPLOAD_MQTT_ID, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "MQTT id"},
    {MENU_REMOTE_UPLOAD_MQTT_USERNAME, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "MQTT user"},
    {MENU_REMOTE_UPLOAD_MQTT_PASSWORD, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "MQTT passwd"},
    {MENU_REMOTE_UPLOAD_MQTT_TOPIC, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "MQTT topic"},
    {MENU_REMOTE_UPLOAD_CONTRIBUTE_ANONYMOUS_DATA_TO_EVDASH_DEV_TEAM, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "Contribute anon.data"},
    {MENU_REMOTE_UPLOAD_CONTRIBUTE_ONCE, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "Contribute once"},
    {MENU_REMOTE_UPLOAD_LOGS_TO_EVDASH_SERVER, MENU_REMOTE_UPLOAD, MENU_NO_MENU, "Upload logs to evdash server"},

    {MENU_GPS_TOP, MENU_GPS, MENU_OTHERS, "<- parent menu"},
    {MENU_GPS_MODULE_TYPE, MENU_GPS, MENU_NO_MENU, "GPS module"},
    {MENU_GPS_PORT, MENU_GPS, MENU_NO_MENU, "GPS port"},
    {MENU_GPS_SPEED, MENU_GPS, MENU_NO_MENU, "GPS speed"},
    {MENU_CAR_SPEED_TYPE, MENU_GPS, MENU_NO_MENU, "Car speed"},

    {DEFAULT_SCREEN_TOP, MENU_DEFAULT_SCREEN, MENU_OTHERS, "<- parent menu"},
    {DEFAULT_SCREEN_AUTOMODE, MENU_DEFAULT_SCREEN, MENU_NO_MENU, "Auto mode"},
    {DEFAULT_SCREEN_BASIC_INFO, MENU_DEFAULT_SCREEN, MENU_NO_MENU, "Basic info"},
    {DEFAULT_SCREEN_SPEED, MENU_DEFAULT_SCREEN, MENU_NO_MENU, "Speed"},
    {DEFAULT_SCREEN_BATT_CELL, MENU_DEFAULT_SCREEN, MENU_NO_MENU, "Battery cells"},
    {DEFAULT_SCREEN_CHG_GRAPH, MENU_DEFAULT_SCREEN, MENU_NO_MENU, "Charging graph"},
    {DEFAULT_SCREEN_HUD, MENU_DEFAULT_SCREEN, MENU_NO_MENU, "HUD"},

    {DISTANCE_UNIT_TOP, MENU_DISTANCE_UNIT, MENU_UNITS, "<- parent menu"},
    {DISTANCE_UNIT_KM, MENU_DISTANCE_UNIT, MENU_NO_MENU, "Kilometers"},
    {DISTANCE_UNIT_MI, MENU_DISTANCE_UNIT, MENU_NO_MENU, "Miles"},

    {TEMPERATURE_UNIT_TOP, MENU_TEMPERATURE_UNIT, MENU_UNITS, "<- parent menu"},
    {TEMPERATURE_UNIT_CEL, MENU_TEMPERATURE_UNIT, MENU_NO_MENU, "Celsius"},
    {TEMPERATURE_UNIT_FAR, MENU_TEMPERATURE_UNIT, MENU_NO_MENU, "Fahrenheit"},

    {PRESURE_UNIT_TOP, MENU_PRESSURE_UNIT, MENU_UNITS, "<- parent menu"},
    {PRESURE_UNIT_BAR, MENU_PRESSURE_UNIT, MENU_NO_MENU, "Bar"},
    {PRESURE_UNIT_PSI, MENU_PRESSURE_UNIT, MENU_NO_MENU, "Psi"},

    {LIST_OF_BLE_DEV_TOP, LIST_OF_BLE_DEV, MENU_TOP_LEVEL, "OBD2 device list"},
    {LIST_OF_BLE_1, LIST_OF_BLE_DEV, MENU_TOP_LEVEL, "<- parent menu"},
    {LIST_OF_BLE_2, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_3, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_4, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_5, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_6, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_7, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_8, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_9, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},
    {LIST_OF_BLE_10, LIST_OF_BLE_DEV, MENU_NO_MENU, "-"},

    {MENU_CAR_COMMANDS_TOP, MENU_CAR_COMMANDS, MENU_TOP_LEVEL, "<- parent menu"}};
