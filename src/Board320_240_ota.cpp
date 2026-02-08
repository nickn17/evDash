/**
 * OTA Update implementation separated from Board320_240.cpp
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include "Board320_240.h"

/**
 * OTA Update
 */
void Board320_240::otaUpdate()
{
// Only for core2
#if defined(BOARD_M5STACK_CORE2) || defined(BOARD_M5STACK_CORES3)
#include "raw_githubusercontent_com.h" // the root certificate is now in const char * root_cert

#ifndef OTA_DIST_DIR
#define OTA_DIST_DIR "m5stack-core2-v1_1"
#endif

  printHeapMemory();

  const String url = String("https://raw.githubusercontent.com/nickn17/evDash/master/dist/") + OTA_DIST_DIR + "/evDash.ino.bin";

  if (!WiFi.isConnected())
  {
    displayMessage("No WiFi connection.", "");
    delay(2000);
    return;
  }
  displayMessage("Downloading OTA...", OTA_DIST_DIR);

  WiFiClientSecure client;
  client.setTimeout(20000);
  client.setCACert(root_cert);

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(20000);

  if (!http.begin(client, url))
  {
    displayMessage("OTA begin failed", "");
    delay(2000);
    return;
  }

  int responseCode = http.GET();
  if (responseCode <= 0)
  {
    // TLS chain on GitHub occasionally changes; fallback to insecure to avoid bricking OTA feature.
    http.end();
    client.stop();
    client.setInsecure();
    if (!http.begin(client, url))
    {
      displayMessage("OTA begin failed", "TLS fallback");
      delay(2000);
      return;
    }
    responseCode = http.GET();
  }

  if (responseCode != HTTP_CODE_OK)
  {
    syslog->print("OTA HTTP code: ");
    syslog->println(responseCode);
    char httpStatus[20];
    snprintf(httpStatus, sizeof(httpStatus), "HTTP %d", responseCode);
    displayMessage("Download failed", httpStatus);
    http.end();
    delay(2500);
    return;
  }

  const int contentLength = http.getSize();
  syslog->print("OTA Content-Length: ");
  syslog->println(contentLength);

  displayMessage("Installing OTA...", "");
  bool beginOk = false;
  if (contentLength > 0)
  {
    beginOk = Update.begin(static_cast<size_t>(contentLength));
  }
  else
  {
    beginOk = Update.begin(UPDATE_SIZE_UNKNOWN);
  }

  if (!beginOk)
  {
    printHeapMemory();
    syslog->print("OTA begin error: ");
    syslog->println(Update.getError());
    displayMessage("Not enough space", "or OTA begin err");
    http.end();
    delay(2500);
    return;
  }

  displayMessage("Writing stream...", "Please wait!");
  WiFiClient *stream = http.getStreamPtr();
  const size_t written = Update.writeStream(*stream);
  if (contentLength > 0 && written != static_cast<size_t>(contentLength))
  {
    syslog->print("OTA short write: ");
    syslog->print(written);
    syslog->print("/");
    syslog->println(contentLength);
    displayMessage("Download incomplete", "");
    Update.abort();
    http.end();
    delay(2500);
    return;
  }

  if (!Update.end(true))
  {
    syslog->print("OTA end error: ");
    syslog->println(Update.getError());
    displayMessage("Downloading error", "OTA end failed");
    http.end();
    delay(2500);
    return;
  }

  http.end();

  if (!Update.isFinished())
  {
    displayMessage("Update not finished.", "Something went wrong.");
    delay(2500);
    return;
  }

  displayMessage("OTA installed.", "Reboot device.");
  delay(2000);
  ESP.restart();

#endif // BOARD_M5STACK_CORE2 || BOARD_M5STACK_CORES3
}
