#pragma once

#include <HardwareSerial.h>
#include <FS.h>
#include <SD.h>

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
  bool logToSdcard = false;
  File file;

public:
  LogSerial();

  //
  void setDebugLevel(uint8_t aDebugLevel);
  void setLogToSdcard(bool state);

  // info
  template <class T, typename... Args>
  void info(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    println(msg);
    if (logToSdcard)
    {
      file = SD.open("/console_output", FILE_APPEND);
      file.println(msg);
      file.close();
    }
  }
  template <class T, typename... Args>
  void infoNolf(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print(msg);
    if (logToSdcard)
    {
      file = SD.open("/console_output", FILE_APPEND);
      file.print(msg);
      file.close();
    }
  }
  // warning
  template <class T, typename... Args>
  void warn(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("WARN ");
    println(msg);
    if (logToSdcard)
    {
      file = SD.open("/console_output", FILE_APPEND);
      file.print("WARN ");
      file.println(msg);
      file.close();
    }
  }
  template <class T, typename... Args>
  void warnNolf(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("WARN ");
    print(msg);
    if (logToSdcard)
    {
      file = SD.open("/console_output", FILE_APPEND);
      file.print("WARN ");
      file.print(msg);
      file.close();
    }
  }

  // error
  template <class T, typename... Args>
  void err(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("ERR ");
    println(msg);
    //
    file = SD.open("/console_output", FILE_APPEND);
    file.print("ERR ");
    file.println(msg);
    file.close();
  }
  template <class T, typename... Args>
  void errNolf(uint8_t aDebugLevel, T msg)
  {
    if (debugLevel != DEBUG_NONE && aDebugLevel != DEBUG_NONE && aDebugLevel != debugLevel)
      return;
    print("ERR ");
    print(msg);
    //
    file = SD.open("/console_output", FILE_APPEND);
    file.print("ERR ");
    file.print(msg);
    file.close();
  }
};
