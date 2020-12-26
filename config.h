#pragma once

#include <BLEDevice.h>

#define APP_VERSION "v2.2.0-dev"
#define APP_RELEASE_DATE "2020-12-14"

// TFT COLORS FOR TTGO
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F      
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */
#define TFT_DEFAULT_BK     0x0000    // 0x38E0
#define TFT_TEMP        0x0000    // NAVY
#define TFT_GREENYELLOW 0xB7E0
#define TFT_DARKRED     0x3800      /* 128,   0,   0 */
#define TFT_DARKRED2    0x1800      /* 128,   0,   0 */
#define TFT_DARKGREEN2  0x01E0      /* 128,   0,   0 */

// COLDGATE COLORS
#define TFT_GRAPH_COLDGATE0_5 0x4208
#define TFT_GRAPH_COLDGATE5_14 0x6000
#define TFT_GRAPH_COLDGATE15_24 0x0008
#define TFT_GRAPH_OPTIMAL25  0x0200
#define TFT_GRAPH_RAPIDGATE35 0x8300

////////////////////////////////////////////////////////////
// SIM800L
/////////////////////////////////////////////////////////////
#define SIM800L_RST 5
#define SIM800L_TIMER 60

// MENU ITEM
typedef struct {
  int16_t id;
  int16_t parentId;
  int16_t targetParentId;
  char title[50];
  char obdMacAddress[20];
  char serviceUUID[40];
} MENU_ITEM;

#define MENU_VEHICLE_TYPE 1
#define MENU_ADAPTER_TYPE 5
#define MENU_SAVE_SETTINGS 9
#define MENU_APP_VERSION 10
//
#define MENU_ADAPTER_BLE4 501
#define MENU_ADAPTER_CAN 502
#define MENU_ADAPTER_BT3 503
//
#define MENU_WIFI 301
#define MENU_GPRS 302
#define MENU_NTP 303
#define MENU_SDCARD 304
#define MENU_SCREEN_ROTATION 305
#define MENU_DEFAULT_SCREEN 306
#define MENU_SCREEN_BRIGHTNESS 307
#define MENU_PREDRAWN_GRAPHS 308
#define MENU_REMOTE_UPLOAD 309
#define MENU_HEADLIGHTS_REMINDER 310
#define MENU_SLEEP_MODE 311
#define MENU_GPS 312
//
#define MENU_DISTANCE_UNIT 401
#define MENU_TEMPERATURE_UNIT 402
#define MENU_PRESSURE_UNIT 403
//
#define MENU_WIFI_ENABLED 3011
#define MENU_WIFI_SSID 3012
#define MENU_WIFI_PASSWORD 3013
//
#define MENU_SDCARD_ENABLED 3041
#define MENU_SDCARD_AUTOSTARTLOG 3042
#define MENU_SDCARD_MOUNT_STATUS 3043
#define MENU_SDCARD_REC 3044
// 
