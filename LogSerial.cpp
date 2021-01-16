#include "LogSerial.h"

/**
 * Constructor
 */
LogSerial::LogSerial() : HardwareSerial(0) {
  //HardwareSerial::begin(115200);  # used syslog->begin(115200); (in evDash.ino)
}

/**
 * Set debug level
 */
void LogSerial::setDebugLevel(uint8_t aDebugLevel) {
  debugLevel = aDebugLevel;
}
