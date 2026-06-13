#ifndef CAR_MODEL_UTILS_H
#define CAR_MODEL_UTILS_H

#include <Arduino.h>
#include "LiveData.h"

// Single source of truth for per-vehicle ids. Both the ABRP telemetry model string
// and the mobile-app relay vehicle id are looked up from one table (kCarModels in
// CarModelUtils.cpp), so adding a car means editing one row instead of keeping two
// parallel switches (here and in EvDashMobileRelay) in sync with the app.
String getCarModelAbrpStr(int carType);   // ABRP car_model, "n/a" if unknown
String getCarModelRelayId(int carType);   // mobile-app vehicle id, "unknown" if unknown

#endif // CAR_MODEL_UTILS_H
