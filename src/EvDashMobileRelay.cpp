#include "EvDashMobileRelay.h"
#include <ArduinoJson.h>
#include <math.h>
#include "BoardInterface.h"
#include "CarModelUtils.h"
#include "config.h"
#include "LogSerial.h"

void EvDashMobileRelay::begin(LiveData *pLiveData, BoardInterface *pBoard)
{
  liveData = pLiveData;
  board = pBoard;
  if (syslog != nullptr)
  {
    syslog->setMirrorCallback(EvDashMobileRelay::serialMirrorThunk, this);
  }
  if (liveData != nullptr && liveData->settings.relayForMobileEnabled == 1)
  {
    startServer();
  }
}

void EvDashMobileRelay::loop()
{
  if (liveData == nullptr)
  {
    return;
  }
  if (netUploadPaused)
  {
    return;
  }
  if (liveData->settings.relayForMobileEnabled == 0)
  {
    stopServer();
    return;
  }
  if (!started)
  {
    startServer();
  }
  const uint32_t nowMs = millis();
  if (serialCaptureEnabled && serialMirrorBuffer.length() > 0 && nowMs - lastSerialMirrorMs > 800)
  {
    flushSerialMirrorLine();
  }
  if (!connected || notifyCharacteristic == nullptr)
  {
    return;
  }
  if (serialCaptureEnabled && (serialPendingLines.length() > 0 || serialPendingOverflow))
  {
    flushSerialPendingLines(kSerialFlushLinesPerLoop);
  }
  if (nowMs - lastHelloMs > 5000)
  {
    sendHello();
    lastHelloMs = nowMs;
  }
  if (!paired())
  {
    return;
  }
  if (nowMs - lastSnapshotMs > 500)
  {
    sendSnapshot();
    lastSnapshotMs = nowMs;
  }
  if (nowMs - lastCellsMs > 3000)
  {
    sendCells();
    lastCellsMs = nowMs;
  }
  if (nowMs - lastTempsMs > 3000)
  {
    sendTemps();
    lastTempsMs = nowMs;
  }
  if (nowMs - lastRawMs > 2500)
  {
    sendRawFrames();
    lastRawMs = nowMs;
  }
}

String EvDashMobileRelay::startPairing()
{
  if (liveData == nullptr)
  {
    return "";
  }
  if (!started)
  {
    startServer();
  }
  snprintf(currentPairingCode, sizeof(currentPairingCode), "%06lu", static_cast<unsigned long>(esp_random() % 1000000UL));
  pairingExpiresAtMs = millis() + 60000UL;
  liveData->settings.relayForMobileEnabled = 1;
  if (board != nullptr)
  {
    board->saveSettings();
  }
  sendHello();
  return String(currentPairingCode);
}

void EvDashMobileRelay::forgetPairing()
{
  if (liveData == nullptr)
  {
    return;
  }
  liveData->settings.relayToken[0] = '\0';
  liveData->settings.relayMobileId[0] = '\0';
  currentPairingCode[0] = '\0';
  pairingExpiresAtMs = 0;
  if (board != nullptr)
  {
    board->saveSettings();
  }
  sendHello();
}

void EvDashMobileRelay::pauseForNetUpload()
{
  netUploadPaused = true;
  if (connected && server != nullptr)
  {
#ifdef EVDASH_USE_NIMBLE
    // NimBLE has no getConnId(); disconnect every connected phone (collect handles
    // first because the peer list shrinks as we disconnect).
    std::vector<uint16_t> handles;
    for (size_t i = 0; i < server->getConnectedCount(); i++)
    {
      handles.push_back(server->getPeerInfo(i).getConnHandle());
    }
    for (uint16_t h : handles)
    {
      server->disconnect(h);
    }
#else
    server->disconnect(server->getConnId());
#endif
    delay(80);
  }
  stopServer();
  if (liveData != nullptr && liveData->settings.commType != COMM_TYPE_OBD2_BLE4 && BLEDevice::getInitialized())
  {
    BLEDevice::deinit(false);
    server = nullptr;
    notifyCharacteristic = nullptr;
    writeCharacteristic = nullptr;
    started = false;
    connected = false;
    if (syslog != nullptr)
    {
      syslog->println("Mobile relay BLE stack released for HTTPS");
    }
  }
}

void EvDashMobileRelay::resumeAfterNetUpload()
{
  netUploadPaused = false;
  if (liveData != nullptr && liveData->settings.relayForMobileEnabled == 1)
  {
    startServer();
  }
}

bool EvDashMobileRelay::clientConnected() const
{
  return connected;
}

const char *EvDashMobileRelay::pairingCode() const
{
  return currentPairingCode;
}

uint16_t EvDashMobileRelay::pairingSecondsLeft() const
{
  if (!pairingOpen())
  {
    return 0;
  }
  return static_cast<uint16_t>((pairingExpiresAtMs - millis()) / 1000UL);
}

void EvDashMobileRelay::onConnect(BLEServer *server)
{
  (void)server;
  connected = true;
  if (syslog != nullptr)
  {
    syslog->println("Mobile relay connected");
  }
  sendHello();
  flushSerialPendingLines(8);
}

void EvDashMobileRelay::onDisconnect(BLEServer *server)
{
  (void)server;
  connected = false;
  if (!serialCaptureEnabled)
  {
    serialMirrorBuffer = "";
    serialPendingLines = "";
    serialPendingOverflow = false;
  }
  if (syslog != nullptr)
  {
    syslog->println("Mobile relay disconnected");
  }
  if (!netUploadPaused)
  {
    BLEDevice::startAdvertising();
  }
}

void EvDashMobileRelay::onWrite(BLECharacteristic *characteristic)
{
#ifdef EVDASH_USE_NIMBLE
  std::string value = characteristic->getValue<std::string>();
#else
  std::string value = characteristic->getValue();
#endif
  if (value.empty())
  {
    return;
  }
  for (size_t i = 0; i < value.length(); i++)
  {
    const char ch = value[i];
    if (ch == '\n')
    {
      handleCommand(rxBuffer);
      rxBuffer = "";
    }
    else if (ch != '\r')
    {
      rxBuffer += ch;
      if (rxBuffer.length() > 512)
      {
        rxBuffer = "";
      }
    }
  }
}

void EvDashMobileRelay::startServer()
{
  if (started)
  {
    return;
  }
  if (server != nullptr)
  {
    BLEDevice::startAdvertising();
    started = true;
    return;
  }
  BLEDevice::init("evDash Relay");
  server = BLEDevice::createServer();
  server->setCallbacks(this);

  BLEService *service = server->createService(EVDASH_RELAY_SERVICE_UUID);
#ifdef EVDASH_USE_NIMBLE
  // NimBLE auto-creates the 0x2902 CCCD for NOTIFY/INDICATE characteristics.
  notifyCharacteristic = service->createCharacteristic(
      EVDASH_RELAY_NOTIFY_UUID,
      NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);
  writeCharacteristic = service->createCharacteristic(
      EVDASH_RELAY_WRITE_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
#else
  esp_ble_gap_set_device_name("evDash Relay");
  notifyCharacteristic = service->createCharacteristic(
      EVDASH_RELAY_NOTIFY_UUID,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  notifyCharacteristic->addDescriptor(new BLE2902());
  writeCharacteristic = service->createCharacteristic(
      EVDASH_RELAY_WRITE_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
#endif
  writeCharacteristic->setCallbacks(this);

  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  if (!advertisingUuidAdded)
  {
    advertising->addServiceUUID(EVDASH_RELAY_SERVICE_UUID);
    advertisingUuidAdded = true;
  }
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  started = true;
  if (syslog != nullptr)
  {
    syslog->println("Mobile relay BLE server started");
  }
}

void EvDashMobileRelay::stopServer()
{
  if (!started)
  {
    return;
  }
  BLEDevice::stopAdvertising();
  started = false;
  connected = false;
  if (syslog != nullptr)
  {
    syslog->println("Mobile relay BLE server stopped");
  }
}

void EvDashMobileRelay::handleCommand(const String &jsonLine)
{
  DynamicJsonDocument doc(384);
  DeserializationError error = deserializeJson(doc, jsonLine);
  if (error)
  {
    notifyJson("{\"type\":\"error\",\"ver\":2,\"message\":\"badJson\"}");
    return;
  }
  String type = doc["type"] | "";
  if (type == "pairStart")
  {
    handlePairStart(String(doc["mobileId"] | ""), String(doc["code"] | ""));
    return;
  }
  String token = doc["token"] | "";
  if (!paired() || token != String(liveData->settings.relayToken))
  {
    notifyJson("{\"type\":\"pairRequired\",\"ver\":2}");
    return;
  }
  if (type == "hello" || type == "ping" || type == "requestFullSnapshot")
  {
    sendHello();
    sendSnapshot();
    sendCells();
    sendTemps();
    sendRawFrames();
  }
  else if (type == "requestContribute")
  {
    sendContributePayload();
  }
  else if (type == "serialCaptureStart")
  {
    handleSerialCaptureStart();
  }
  else if (type == "serialCaptureStop")
  {
    handleSerialCaptureStop();
  }
  else if (type == "forgetPair")
  {
    forgetPairing();
  }
}

void EvDashMobileRelay::handlePairStart(const String &mobileId, const String &code)
{
  if (!pairingOpen() || code != String(currentPairingCode))
  {
    notifyJson("{\"type\":\"pairRequired\",\"ver\":2}");
    return;
  }
  String token = ensureRelayToken();
  mobileId.toCharArray(liveData->settings.relayMobileId, sizeof(liveData->settings.relayMobileId));
  currentPairingCode[0] = '\0';
  pairingExpiresAtMs = 0;
  if (board != nullptr)
  {
    board->saveSettings();
  }

  String json = "{\"type\":\"pairOk\",\"ver\":2,\"relayId\":\"" + relayId() +
                "\",\"mobileId\":\"" + escapeJson(String(liveData->settings.relayMobileId)) +
                "\",\"token\":\"" + escapeJson(token) +
                "\",\"vehicleId\":\"" + vehicleId() + "\"}";
  notifyJson(json);
}

void EvDashMobileRelay::handleSerialCaptureStart()
{
  serialMirrorBuffer = "";
  serialCaptureEnabled = true;
  lastSerialMirrorMs = millis();
  notifyJson("{\"type\":\"serialCapture\",\"ver\":2,\"state\":\"started\"}");
}

void EvDashMobileRelay::handleSerialCaptureStop()
{
  flushSerialMirrorLine();
  while (connected && notifyCharacteristic != nullptr && (serialPendingLines.length() > 0 || serialPendingOverflow))
  {
    flushSerialPendingLines(255);
  }
  serialCaptureEnabled = false;
  serialPendingLines = "";
  serialPendingOverflow = false;
  notifyJson("{\"type\":\"serialCapture\",\"ver\":2,\"state\":\"stopped\"}");
}

void EvDashMobileRelay::notifyLine(const String &line)
{
  if (!connected || notifyCharacteristic == nullptr)
  {
    return;
  }
  String payload = line + "\n";
  const size_t chunkSize = 160;
  for (size_t pos = 0; pos < payload.length(); pos += chunkSize)
  {
    String chunk = payload.substring(pos, pos + chunkSize);
    notifyCharacteristic->setValue(reinterpret_cast<uint8_t *>(const_cast<char *>(chunk.c_str())), chunk.length());
    notifyCharacteristic->notify();
    delay(3);
  }
}

void EvDashMobileRelay::notifyJson(const String &json)
{
  notifyLine(json);
}

void EvDashMobileRelay::sendHello()
{
  if (liveData == nullptr)
  {
    return;
  }
  String json = "{\"type\":\"hello\",\"ver\":2,\"relayId\":\"" + relayId() +
                "\",\"fw\":\"" + String(APP_VERSION) +
                "\",\"vehicleId\":\"" + vehicleId() +
                "\",\"paired\":" + jsonBool(paired()) +
                ",\"pairing\":" + jsonBool(pairingOpen()) + "}";
  notifyJson(json);
}

void EvDashMobileRelay::sendContributePayload()
{
  if (board == nullptr)
  {
    return;
  }
  String json;
  if (board->buildContributePayloadV2(json, false) && json.length() > 2)
  {
    notifyJson(json);
  }
}

void EvDashMobileRelay::sendSnapshot()
{
  PARAMS_STRUC &p = liveData->params;
  String json = "{\"type\":\"snapshot\",\"ver\":2";
  json += ",\"ts\":" + String(static_cast<uint32_t>(p.currentTime));
  json += ",\"relayId\":\"" + relayId() + "\"";
  json += ",\"vehicleId\":\"" + vehicleId() + "\"";
  json += ",\"carType\":" + String(liveData->settings.carType);
  json += ",\"comm\":\"" + commType() + "\"";
  json += ",\"gpsSource\":\"m5stack\"";
  json += ",\"vin\":\"" + escapeJson(String(p.carVin)) + "\"";
  json += ",\"spd\":" + jsonNumber(p.speedKmh, 1);
  json += ",\"gpsSpd\":" + jsonNumber(p.speedKmhGPS, 1);
  json += ",\"soc\":" + jsonNumber(p.socPerc, 1);
  json += ",\"socBms\":" + jsonNumber(p.socPercBms, 1);
  json += ",\"soh\":" + jsonNumber(p.sohPerc, 1);
  json += ",\"powKw\":" + jsonNumber(p.batPowerKw, 2);
  json += ",\"powKwh100\":" + jsonNumber(p.batPowerKwh100, 2);
  json += ",\"batV\":" + jsonNumber(p.batVoltage, 1);
  json += ",\"batA\":" + jsonNumber(p.batPowerAmp, 1);
  json += ",\"auxV\":" + jsonNumber(p.auxVoltage, 1);
  json += ",\"auxPct\":" + jsonNumber(p.auxPerc, 0);
  json += ",\"odoKm\":" + jsonNumber(p.odoKm, 1);
  json += ",\"avgSpd\":" + jsonNumber(p.avgSpeedKmh, 1);
  json += ",\"tripKm\":";
  json += (p.odoKm >= 0.0f && p.odoKmStart >= 0.0f) ? jsonNumber(p.odoKm - p.odoKmStart, 1) : "null";
  json += ",\"inC\":";
  json += (p.indoorTemperature > -99.0f) ? jsonNumber(p.indoorTemperature, 1) : "null";
  json += ",\"outC\":";
  json += (p.outdoorTemperature > -99.0f) ? jsonNumber(p.outdoorTemperature, 1) : "null";
  json += ",\"chg\":" + jsonBool(p.chargingOn);
  json += ",\"chgAc\":" + jsonBool(p.chargerACconnected);
  json += ",\"chgDc\":" + jsonBool(p.chargerDCconnected);
  json += ",\"drive\":\"" + driveMode() + "\"";
  json += ",\"batMinC\":" + jsonNumber(p.batMinC, 0);
  json += ",\"batMaxC\":" + jsonNumber(p.batMaxC, 0);
  json += ",\"batInletC\":" + jsonNumber(p.batInletC, 0);
  json += ",\"batHeaterC\":" + jsonNumber(p.batHeaterC, 0);
  json += ",\"coolantC\":" + jsonNumber(p.coolingWaterTempC, 0);
  json += ",\"cMinV\":" + jsonNumber(p.batCellMinV, 3);
  json += ",\"cMaxV\":" + jsonNumber(p.batCellMaxV, 3);
  json += ",\"cMinNo\":" + String(p.batCellMinVNo);
  json += ",\"cMaxNo\":" + String(p.batCellMaxVNo);
  json += ",\"chargedKwh\":";
  json += (p.cumulativeEnergyChargedKWh >= 0.0f && p.cumulativeEnergyChargedKWhStart >= 0.0f) ? jsonNumber(p.cumulativeEnergyChargedKWh - p.cumulativeEnergyChargedKWhStart, 2) : "null";
  json += ",\"dischargedKwh\":";
  json += (p.cumulativeEnergyDischargedKWh >= 0.0f && p.cumulativeEnergyDischargedKWhStart >= 0.0f) ? jsonNumber(p.cumulativeEnergyDischargedKWh - p.cumulativeEnergyDischargedKWhStart, 2) : "null";
  json += ",\"availKwh\":" + jsonNumber(p.batteryTotalAvailableKWh, 1);
  json += ",\"batKwh\":" + jsonNumber(p.batEnergyContent, 1);
  json += ",\"batMaxKwh\":" + jsonNumber(p.batMaxEnergyContent, 1);
  json += ",\"bms\":\"" + bmsMode() + "\"";
  json += ",\"flDoor\":" + jsonBool(p.leftFrontDoorOpen);
  json += ",\"frDoor\":" + jsonBool(p.rightFrontDoorOpen);
  json += ",\"rlDoor\":" + jsonBool(p.leftRearDoorOpen);
  json += ",\"rrDoor\":" + jsonBool(p.rightRearDoorOpen);
  json += ",\"hood\":" + jsonBool(p.hoodDoorOpen);
  json += ",\"trunk\":" + jsonBool(p.trunkDoorOpen);
  json += ",\"headlights\":" + jsonBool(p.headLights || p.dayLights || p.autoLights);
  json += ",\"brakeLights\":" + jsonBool(p.brakeLights);
  json += ",\"frontRpm\":" + jsonNumber(p.motor1Rpm, 0);
  json += ",\"rearRpm\":" + jsonNumber(p.motor2Rpm, 0);
  // TPMS — convert bar -> kPa (Flutter side expects kPa). null when sensor
  // hasn't reported yet (firmware uses 0/-99 sentinels) so the iPhone shows
  // "--" instead of a bogus 0.
  json += ",\"flTireKpa\":";
  json += (p.tireFrontLeftPressureBar > 0.05f) ? jsonNumber(p.tireFrontLeftPressureBar * 100.0f, 0) : "null";
  json += ",\"frTireKpa\":";
  json += (p.tireFrontRightPressureBar > 0.05f) ? jsonNumber(p.tireFrontRightPressureBar * 100.0f, 0) : "null";
  json += ",\"rlTireKpa\":";
  json += (p.tireRearLeftPressureBar > 0.05f) ? jsonNumber(p.tireRearLeftPressureBar * 100.0f, 0) : "null";
  json += ",\"rrTireKpa\":";
  json += (p.tireRearRightPressureBar > 0.05f) ? jsonNumber(p.tireRearRightPressureBar * 100.0f, 0) : "null";
  json += ",\"flTireC\":";
  json += (p.tireFrontLeftTempC > -50.0f) ? jsonNumber(p.tireFrontLeftTempC, 0) : "null";
  json += ",\"frTireC\":";
  json += (p.tireFrontRightTempC > -50.0f) ? jsonNumber(p.tireFrontRightTempC, 0) : "null";
  json += ",\"rlTireC\":";
  json += (p.tireRearLeftTempC > -50.0f) ? jsonNumber(p.tireRearLeftTempC, 0) : "null";
  json += ",\"rrTireC\":";
  json += (p.tireRearRightTempC > -50.0f) ? jsonNumber(p.tireRearRightTempC, 0) : "null";
  json += ",\"lat\":" + jsonNumber(p.gpsLat, 5);
  json += ",\"lon\":" + jsonNumber(p.gpsLon, 5);
  json += ",\"alt\":" + jsonNumber(p.gpsAlt, 0);
  json += ",\"sat\":" + String(p.gpsSat);
  json += ",\"hdg\":" + jsonNumber(p.gpsHeadingDeg, 1);
  json += "}";
  notifyJson(json);
}

void EvDashMobileRelay::sendCells()
{
  PARAMS_STRUC &p = liveData->params;
  const uint16_t count = (p.cellCount > 0 && p.cellCount <= 192) ? p.cellCount : 192;
  String json = "{\"type\":\"cells\",\"ver\":2,\"cells\":[";
  bool first = true;
  for (uint16_t i = 0; i < count; i++)
  {
    const float value = p.cellVoltage[i];
    if (!isfinite(value) || value < 2.5f || value > 4.4f)
    {
      continue;
    }
    if (!first)
    {
      json += ",";
    }
    json += jsonNumber(value, 3);
    first = false;
  }
  json += "]}";
  notifyJson(json);
}

void EvDashMobileRelay::sendTemps()
{
  PARAMS_STRUC &p = liveData->params;
  const uint16_t count = (p.batModuleTempCount > 0 && p.batModuleTempCount <= 25) ? p.batModuleTempCount : 25;
  String json = "{\"type\":\"temps\",\"ver\":2,\"modules\":[";
  bool first = true;
  for (uint16_t i = 0; i < count; i++)
  {
    const float value = p.batModuleTempC[i];
    if (!isfinite(value) || value < -30.0f || value > 80.0f)
    {
      continue;
    }
    if (!first)
    {
      json += ",";
    }
    json += jsonNumber(value, 0);
    first = false;
  }
  json += "]}";
  notifyJson(json);
}

void EvDashMobileRelay::sendRawFrames()
{
  if (liveData->contributeRawFrameCount == 0)
  {
    return;
  }
  String json = "{\"type\":\"raw\",\"ver\":2,\"frames\":{";
  uint8_t added = 0;
  for (uint8_t i = 0; i < liveData->contributeRawFrameCount && added < 8; i++)
  {
    LiveData::ContributeRawFrame &raw = liveData->contributeRawFrames[i];
    if (raw.key[0] == '\0' || raw.value[0] == '\0')
    {
      continue;
    }
    if (added > 0)
    {
      json += ",";
    }
    String key = escapeJson(String(raw.key));
    json += "\"" + key + "\":\"" + escapeJson(String(raw.value)) + "\"";
    json += ",\"" + key + "_ms\":" + String(raw.latencyMs);
    added++;
  }
  json += "}}";
  notifyJson(json);
}

void EvDashMobileRelay::sendSerialLine(const String &line)
{
  if (line.length() == 0)
  {
    return;
  }
  String json = "{\"type\":\"serial\",\"ver\":2,\"uptimeMs\":" + String(millis()) +
                ",\"line\":\"" + escapeJson(line) + "\"}";
  notifyJson(json);
}

void EvDashMobileRelay::queueSerialLine(const String &line)
{
  if (!serialCaptureEnabled || line.length() == 0)
  {
    return;
  }
  String queuedLine = line;
  queuedLine.replace('\r', ' ');
  queuedLine.replace('\n', ' ');
  if (queuedLine.length() > 220)
  {
    queuedLine = queuedLine.substring(0, 220);
  }
  while (serialPendingLines.length() + queuedLine.length() + 1 > kSerialPendingMaxBytes)
  {
    int nextLinePos = serialPendingLines.indexOf('\n');
    if (nextLinePos < 0)
    {
      serialPendingLines = "";
      break;
    }
    serialPendingLines.remove(0, nextLinePos + 1);
    serialPendingOverflow = true;
  }
  serialPendingLines += queuedLine;
  serialPendingLines += "\n";
}

void EvDashMobileRelay::flushSerialPendingLines(uint8_t maxLines)
{
  if (!connected || notifyCharacteristic == nullptr || maxLines == 0)
  {
    return;
  }
  uint8_t sent = 0;
  if (serialPendingOverflow)
  {
    sendSerialLine("serial backlog truncated");
    serialPendingOverflow = false;
    sent++;
  }
  while (sent < maxLines && serialPendingLines.length() > 0)
  {
    int nextLinePos = serialPendingLines.indexOf('\n');
    String line = (nextLinePos >= 0) ? serialPendingLines.substring(0, nextLinePos) : serialPendingLines;
    if (nextLinePos >= 0)
    {
      serialPendingLines.remove(0, nextLinePos + 1);
    }
    else
    {
      serialPendingLines = "";
    }
    line.trim();
    if (line.length() > 0)
    {
      sendSerialLine(line);
      sent++;
    }
  }
}

void EvDashMobileRelay::handleSerialMirror(const uint8_t *data, size_t size)
{
  if (!serialCaptureEnabled || data == nullptr || size == 0)
  {
    return;
  }
  lastSerialMirrorMs = millis();
  for (size_t i = 0; i < size; i++)
  {
    const char ch = static_cast<char>(data[i]);
    if (ch == '\n')
    {
      flushSerialMirrorLine();
    }
    else if (ch != '\r')
    {
      serialMirrorBuffer += ch;
      if (serialMirrorBuffer.length() >= 220)
      {
        flushSerialMirrorLine();
      }
    }
  }
}

void EvDashMobileRelay::flushSerialMirrorLine()
{
  String line = serialMirrorBuffer;
  serialMirrorBuffer = "";
  line.trim();
  if (line.length() > 0)
  {
    queueSerialLine(line);
  }
}

bool EvDashMobileRelay::paired() const
{
  return liveData != nullptr && strlen(liveData->settings.relayToken) >= 12;
}

bool EvDashMobileRelay::pairingOpen() const
{
  return currentPairingCode[0] != '\0' && millis() < pairingExpiresAtMs;
}

String EvDashMobileRelay::relayId() const
{
  char tmp[24];
  snprintf(tmp, sizeof(tmp), "m5-%012llX", static_cast<unsigned long long>(ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL));
  return String(tmp);
}

String EvDashMobileRelay::ensureRelayToken()
{
  if (paired())
  {
    return String(liveData->settings.relayToken);
  }
  String token = generateToken();
  token.toCharArray(liveData->settings.relayToken, sizeof(liveData->settings.relayToken));
  return token;
}

String EvDashMobileRelay::generateToken() const
{
  char token[32];
  snprintf(token, sizeof(token), "k%08lX%08lX%08lX",
           static_cast<unsigned long>(ESP.getEfuseMac() & 0xFFFFFFFFUL),
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(millis()));
  return String(token);
}

String EvDashMobileRelay::vehicleId() const
{
  // Looked up from the single per-vehicle table in CarModelUtils (kept in sync with
  // the ABRP id and the evDash app's vehicle ids).
  return getCarModelRelayId(liveData->settings.carType);
}

String EvDashMobileRelay::commType() const
{
  switch (liveData->settings.commType)
  {
  case COMM_TYPE_CAN_COMMU:
    return "can";
  case COMM_TYPE_OBD2_BLE4:
    return "ble4";
  default:
    return "unknown";
  }
}

String EvDashMobileRelay::driveMode() const
{
  if (liveData->params.forwardDriveMode)
    return "D";
  if (liveData->params.reverseDriveMode)
    return "R";
  if (liveData->params.parkModeOrNeutral)
    return "P/N";
  return "-";
}

String EvDashMobileRelay::bmsMode() const
{
  return liveData->getBatteryManagementModeStr(liveData->params.batteryManagementMode);
}

String EvDashMobileRelay::jsonNumber(float value, uint8_t digits) const
{
  if (!isfinite(value))
  {
    return "null";
  }
  return String(value, static_cast<unsigned int>(digits));
}

String EvDashMobileRelay::jsonBool(bool value) const
{
  return value ? "true" : "false";
}

String EvDashMobileRelay::escapeJson(const String &value) const
{
  String out = "";
  for (uint16_t i = 0; i < value.length(); i++)
  {
    char ch = value.charAt(i);
    if (ch == '"' || ch == '\\')
    {
      out += '\\';
      out += ch;
    }
    else if (ch >= 32)
    {
      out += ch;
    }
  }
  return out;
}

void EvDashMobileRelay::serialMirrorThunk(const uint8_t *data, size_t size, void *context)
{
  if (context != nullptr)
  {
    static_cast<EvDashMobileRelay *>(context)->handleSerialMirror(data, size);
  }
}
