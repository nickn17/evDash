#include "traccar.h"

#include <HTTPClient.h>
#include <WiFiClient.h>

namespace Traccar
{
  bool sendPosition(const char *serverHost,
                    uint16_t serverPort,
                    const String &deviceId,
                    time_t timestamp,
                    double latitude,
                    double longitude,
                    float speedKmh,
                    float altitudeMeters,
                    float headingDeg,
                    float socPercent,
                    bool charging,
                    int &outHttpCode)
  {
    outHttpCode = -1;

    if (serverHost == nullptr || serverHost[0] == '\0' || deviceId.length() == 0)
    {
      return false;
    }

    char url[384] = {0};
    const float speedKnots = speedKmh * 0.539957f;
    const int chars = snprintf(url,
                               sizeof(url),
                               "http://%s:%u/?id=%s&timestamp=%lu&lat=%.6f&lon=%.6f&speed=%.2f&bearing=%.1f&altitude=%.1f&batt=%.1f&charge=%d",
                               serverHost,
                               static_cast<unsigned int>(serverPort),
                               deviceId.c_str(),
                               static_cast<unsigned long>(timestamp),
                               latitude,
                               longitude,
                               speedKnots,
                               headingDeg,
                               altitudeMeters,
                               socPercent,
                               charging ? 1 : 0);

    if (chars <= 0 || static_cast<size_t>(chars) >= sizeof(url))
    {
      return false;
    }

    WiFiClient client;
    HTTPClient http;

    if (!http.begin(client, url))
    {
      return false;
    }

    http.setConnectTimeout(1500);
    http.setTimeout(2500);
    http.setReuse(false);

    outHttpCode = http.GET();
    http.end();

    return (outHttpCode == HTTP_CODE_OK);
  }
}
