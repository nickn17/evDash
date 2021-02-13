#pragma once

#include <HardwareSerial.h>

// DEBUG LEVEL
#define DEBUG_NONE 0
#define DEBUG_COMM 1   // filter comm
#define DEBUG_GSM 2    // filter gsm messages
#define DEBUG_SDCARD 3 // filter sdcard
#define DEBUG_GPS 4    // filter gps

//
class LogSerial : public HardwareSerial
{
protected:
  uint8_t debugLevel;

public:
  LogSerial();
  //
  void setDebugLevel(uint8_t aDebugLevel);
  // info
  template <class T, typename... Args>
  void info(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    println(msg);
  }
  template <class T, typename... Args>
  void infoNolf(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print(msg);
  }
  // warning
  template <class T, typename... Args>
  void warn(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("WARN ");
    println(msg);
  }
  template <class T, typename... Args>
  void warnNolf(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("WARN ");
    print(msg);
  }

  // error
  template <class T, typename... Args>
  void err(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("ERR ");
    println(msg);
  }
  template <class T, typename... Args>
  void errNolf(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("ERR ");
    print(msg);
  }
};
