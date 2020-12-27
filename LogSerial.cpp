#include "LogSerial.h"

/**
 * Constructor
 */
LogSerial::LogSerial() : HardwareSerial(0) {   
  HardwareSerial::begin(115200);
}
