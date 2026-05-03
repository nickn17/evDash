#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "LiveData.h"

class BoardInterface;

/**
 * BLE relay for the iOS/Android evDash app.
 *
 * M5Stack keeps talking to the car. The phone app connects here and receives
 * snapshots, cells, module temperatures, and recent raw frames.
 */
class EvDashMobileRelay : public BLEServerCallbacks, public BLECharacteristicCallbacks
{
public:
  void begin(LiveData *pLiveData, BoardInterface *pBoard);
  void loop();
  String startPairing();
  void forgetPairing();
  const char *pairingCode() const;
  uint16_t pairingSecondsLeft() const;

  void onConnect(BLEServer *server) override;
  void onDisconnect(BLEServer *server) override;
  void onWrite(BLECharacteristic *characteristic) override;

private:
  LiveData *liveData = nullptr;
  BoardInterface *board = nullptr;
  BLEServer *server = nullptr;
  BLECharacteristic *notifyCharacteristic = nullptr;
  BLECharacteristic *writeCharacteristic = nullptr;
  bool started = false;
  bool connected = false;
  char currentPairingCode[7] = "";
  uint32_t pairingExpiresAtMs = 0;
  uint32_t lastHelloMs = 0;
  uint32_t lastSnapshotMs = 0;
  uint32_t lastCellsMs = 0;
  uint32_t lastTempsMs = 0;
  uint32_t lastRawMs = 0;
  uint32_t lastContributeMs = 0;
  String rxBuffer = "";

  void startServer();
  void stopServer();
  void handleCommand(const String &jsonLine);
  void handlePairStart(const String &mobileId, const String &code);
  void notifyLine(const String &line);
  void notifyJson(const String &json);
  void sendHello();
  void sendContributePayload();
  void sendSnapshot();
  void sendCells();
  void sendTemps();
  void sendRawFrames();
  bool paired() const;
  bool pairingOpen() const;
  String relayId() const;
  String ensureRelayToken();
  String generateToken() const;
  String vehicleId() const;
  String commType() const;
  String driveMode() const;
  String bmsMode() const;
  String jsonNumber(float value, uint8_t digits = 1) const;
  String jsonBool(bool value) const;
  String escapeJson(const String &value) const;
};
