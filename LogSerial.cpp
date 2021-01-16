#include "LogSerial.h"

/**
 * Constructor
 */
LogSerial::LogSerial() : HardwareSerial(0) {
  HardwareSerial::begin(115200);
}

/**
 * Set debug level
 */
void LogSerial::setDebugLevel(uint8_t aDebugLevel) {
  debugLevel = aDebugLevel;
}
