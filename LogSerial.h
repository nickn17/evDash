#pragma once

#include <HardwareSerial.h>

// DEBUG LEVEL
#define DEBUG_NONE    0
#define DEBUG_COMM    1 // filter comm
#define DEBUG_GPS     2 // filter gps messages
#define DEBUG_SDCARD  3 // filter sdcard

//
class LogSerial: public HardwareSerial {
  protected:

  public:
    LogSerial();
    /*virtual void infoNolfType(String msg, uint8_t debugLevel = TYPE_NONE);
    virtual void infoType(String msg, uint8_t debugLevel = TYPE_NONE);
    /*
      template <class T, typename... Args> void infoNolf(String msg, uint8_t debugLevel = TYPE_NONE);
      template <class T, typename... Args> void info(String msg, uint8_t debugLevel = DEBUG_ALL);
      template <class T, typename... Args> void warnNolf(String msg, uint8_t debugLevel = DEBUG_ALL);
      template <class T, typename... Args> void warn(String msg, uint8_t debugLevel = DEBUG_ALL);
      template <class T, typename... Args> void errNolf(String msg, uint8_t debugLevel = DEBUG_ALL);
      template <class T, typename... Args> void err(String msg, uint8_t debugLevel = DEBUG_ALL);*/

};
