#include "LogSerial.h"

/**
 * Constructor
 */
LogSerial::LogSerial() : HardwareSerial(0)
{
  // HardwareSerial::begin(115200);  # used syslog->begin(115200); (in evDash.ino)
}

/**
 * Set debug level
 */
void LogSerial::setDebugLevel(uint8_t aDebugLevel)
{
  debugLevel = aDebugLevel;
}

/**
 * Write log to sdcard
 */
void LogSerial::setLogToSdcard(bool state)
{
  logToSdcard = state;
  
  if (logToSdcard)
  {
    // code under construction
    file = SD.open("/console_output", FILE_WRITE);
    file.println("========================");
    file.close();
  }
}