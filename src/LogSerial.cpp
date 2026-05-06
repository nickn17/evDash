/**
 * LogSerial class provides logging functionality.
 *
 * Allows setting debug level and logging to SD card.
 */
#include "LogSerial.h"

/**
 * Constructor
 */
#ifndef BOARD_M5STACK_CORES3
LogSerial::LogSerial() : HardwareSerial(0)
{
  // HardwareSerial::begin(115200);  // used syslog->begin(115200); (in evDash.ino)
}
#endif // BOARD_M5STACK_CORES3

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

/**
 * Mirror console bytes to an optional consumer.
 */
void LogSerial::setMirrorCallback(LogSerialMirrorCallback callback, void *context)
{
  mirrorCallback = callback;
  mirrorContext = context;
}

/**
 * Write one byte to console and mirror it when enabled.
 */
size_t LogSerial::write(uint8_t data)
{
#ifdef BOARD_M5STACK_CORES3
  size_t written = HWCDC::write(data);
#else
  size_t written = HardwareSerial::write(data);
#endif // BOARD_M5STACK_CORES3
  if (mirrorCallback != nullptr)
  {
    mirrorCallback(&data, 1, mirrorContext);
  }
  return written;
}

/**
 * Write bytes to console and mirror them when enabled.
 */
size_t LogSerial::write(const uint8_t *buffer, size_t size)
{
#ifdef BOARD_M5STACK_CORES3
  size_t written = HWCDC::write(buffer, size);
#else
  size_t written = HardwareSerial::write(buffer, size);
#endif // BOARD_M5STACK_CORES3
  if (mirrorCallback != nullptr && buffer != nullptr && size > 0)
  {
    mirrorCallback(buffer, size, mirrorContext);
  }
  return written;
}
