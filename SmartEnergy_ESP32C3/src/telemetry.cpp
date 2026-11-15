#include "telemetry.h"

#include <math.h>
#include <string.h>
#include <ArduinoJson.h>

TelemetryProducer::TelemetryProducer()
    : serialPort(nullptr), txHead(0), txTail(0), txCount(0), nextSequence(0) {
  memset(txBuffer, 0, sizeof(txBuffer));
  memset(txLengths, 0, sizeof(txLengths));
  memset(txOffsets, 0, sizeof(txOffsets));
}

void TelemetryProducer::begin(HardwareSerial& serialPortRef) {
  serialPort = &serialPortRef;
  serialPort->begin(115200, SERIAL_8N1, 20, 21);
}

bool TelemetryProducer::transmit(const TelemetryPayload& payload) {
  if (!serialPort || payload.timestamp[0] == '\0' ||
      !isfinite(payload.voltage) || !isfinite(payload.current) ||
      !isfinite(payload.power) || !isfinite(payload.energy) ||
      !isfinite(payload.temperature) || payload.status[0] == '\0') {
    return false;
  }

  StaticJsonDocument<256> document;
  document["ts"] = payload.timestamp;
  document["seq"] = nextSequence;
  document["voltage"] = payload.voltage;
  document["current"] = payload.current;
  document["power"] = payload.power;
  document["energy"] = payload.energy;
  document["temperature"] = payload.temperature;
  document["status"] = payload.status;
  document["daily_energy"] = payload.dailyEnergy;
  document["weekly_energy"] = payload.weeklyEnergy;
  document["monthly_energy"] = payload.monthlyEnergy;

  const size_t jsonLength = serializeJson(document, txBuffer, sizeof(txBuffer) - 1);
  if (jsonLength == 0 || jsonLength >= sizeof(txBuffer) - 1) return false;
  txBuffer[jsonLength] = '\n';
  const size_t frameLength = jsonLength + 1;
  if (txCount >= kTxQueueCapacity) return false;
  memcpy(txQueue[txHead], txBuffer, frameLength);
  txLengths[txHead] = static_cast<uint16_t>(frameLength);
  txOffsets[txHead] = 0;
  txHead = (txHead + 1) % kTxQueueCapacity;
  ++txCount;
  ++nextSequence;
  return true;
}

void TelemetryProducer::service() {
  if (!serialPort || txCount == 0) return;
  const size_t available = serialPort->availableForWrite();
  const size_t remaining = txLengths[txTail] - txOffsets[txTail];
  const size_t chunkLength = available < remaining ? available : remaining;
  if (chunkLength == 0) return;

  const size_t written = serialPort->write(
      reinterpret_cast<const uint8_t*>(&txQueue[txTail][txOffsets[txTail]]), chunkLength);
  txOffsets[txTail] += static_cast<uint16_t>(written);
  if (txOffsets[txTail] >= txLengths[txTail]) {
    txLengths[txTail] = 0;
    txOffsets[txTail] = 0;
    txTail = (txTail + 1) % kTxQueueCapacity;
    --txCount;
  }
}

bool TelemetryProducer::parseIncomingLine(const char* line, TelemetryPayload& payload) {
  if (line == nullptr) {
    return false;
  }

  StaticJsonDocument<256> document;
  if (deserializeJson(document, line) || !document["ts"].is<const char*>()) return false;
  const char* timestamp = document["ts"].as<const char*>();
  if (strlen(timestamp) != 19 || timestamp[4] != '-' || timestamp[7] != '-' ||
      timestamp[10] != ' ' || timestamp[13] != ':' || timestamp[16] != ':' ||
      !document["status"].is<const char*>()) return false;
  const char* status = document["status"].as<const char*>();
  const size_t statusLength = strlen(status);
  if (statusLength == 0 || statusLength >= sizeof(payload.status) ||
      (strcmp(status, "GREEN") != 0 && strcmp(status, "YELLOW") != 0 &&
       strcmp(status, "RED") != 0 && strcmp(status, "FAULT") != 0)) return false;

  const char* required[] = {"voltage", "current", "power", "energy", "temperature"};
  for (const char* key : required) if (!document[key].is<float>()) return false;
  payload.voltage = document["voltage"].as<float>();
  payload.current = document["current"].as<float>();
  payload.power = document["power"].as<float>();
  payload.energy = document["energy"].as<float>();
  payload.temperature = document["temperature"].as<float>();
  payload.dailyEnergy = document["daily_energy"] | 0.0f;
  payload.weeklyEnergy = document["weekly_energy"] | 0.0f;
  payload.monthlyEnergy = document["monthly_energy"] | 0.0f;
  payload.sequence = document["seq"] | 0UL;
  if (!isfinite(payload.voltage) || !isfinite(payload.current) || !isfinite(payload.power) ||
      !isfinite(payload.energy) || !isfinite(payload.temperature) ||
      !isfinite(payload.dailyEnergy) || !isfinite(payload.weeklyEnergy) ||
      !isfinite(payload.monthlyEnergy) || payload.voltage < 0.0f || payload.voltage > 400.0f ||
      payload.current < 0.0f || payload.current > 200.0f || payload.power < -80000.0f ||
      payload.power > 80000.0f || payload.energy < 0.0f || payload.dailyEnergy < 0.0f ||
      payload.weeklyEnergy < 0.0f || payload.monthlyEnergy < 0.0f ||
      payload.temperature < (strcmp(status, "FAULT") == 0 ? -127.0f : -55.0f) ||
      payload.temperature > 125.0f) return false;

  snprintf(payload.timestamp, sizeof(payload.timestamp), "%s", timestamp);
  memcpy(payload.status, status, statusLength + 1);
  return true;
}
