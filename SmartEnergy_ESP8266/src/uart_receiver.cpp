#include "uart_receiver.h"

#include <ArduinoJson.h>
#include <math.h>
#include <string.h>

UartReceiver::UartReceiver()
    : serialPort(nullptr), rxIndex(0), validPacket(false), discardingFrame(false) {
  memset(rxBuffer, 0, sizeof(rxBuffer));
}

void UartReceiver::begin(HardwareSerial& serialPortRef) {
  serialPort = &serialPortRef;
  serialPort->begin(115200);
}

bool UartReceiver::receivePacket() {
  if (!serialPort) {
    return false;
  }

  while (serialPort->available()) {
    const char c = static_cast<char>(serialPort->read());

    if (c == '\n') {
      if (discardingFrame) {
        discardingFrame = false;
        rxIndex = 0;
        continue;
      }
      if (rxIndex > 0 && rxBuffer[rxIndex - 1] == '\r') --rxIndex;
      rxBuffer[rxIndex] = '\0';
      rxIndex = 0;
      validPacket = (strlen(rxBuffer) > 0);
      return validPacket;
    }

    if (discardingFrame) continue;
    if (rxIndex < sizeof(rxBuffer) - 1) {
      rxBuffer[rxIndex++] = c;
    } else {
      discardingFrame = true;
      rxIndex = 0;
      validPacket = false;
    }
  }

  return false;
}

const char* UartReceiver::getLastPacket() const {
  return rxBuffer;
}

bool UartReceiver::parseTelemetry(const char* json, UartTelemetry& out) {
  if (!json) {
    return false;
  }

  StaticJsonDocument<256> document;
  if (deserializeJson(document, json) || !document["ts"].is<const char*>() ||
      !document["status"].is<const char*>()) return false;
  const char* timestamp = document["ts"].as<const char*>();
  const char* status = document["status"].as<const char*>();
  const size_t timestampLength = strlen(timestamp);
  const size_t statusLength = strlen(status);
  const bool timestampValid = strcmp(timestamp, "UNSYNCED") == 0 ||
      (timestampLength == 19 && timestamp[4] == '-' && timestamp[7] == '-' &&
       timestamp[10] == ' ' && timestamp[13] == ':' && timestamp[16] == ':');
  if (!timestampValid ||
      statusLength == 0 || statusLength >= sizeof(out.status) ||
      (strcmp(status, "GREEN") != 0 && strcmp(status, "YELLOW") != 0 &&
       strcmp(status, "RED") != 0 && strcmp(status, "FAULT") != 0)) return false;

  const char* required[] = {"voltage", "current", "power", "energy", "temperature"};
  for (const char* key : required) if (!document[key].is<float>()) return false;
  out.voltage = document["voltage"].as<float>();
  out.current = document["current"].as<float>();
  out.power = document["power"].as<float>();
  out.energy = document["energy"].as<float>();
  out.temperature = document["temperature"].as<float>();
  out.dailyEnergy = document["daily_energy"] | 0.0f;
  out.weeklyEnergy = document["weekly_energy"] | 0.0f;
  out.monthlyEnergy = document["monthly_energy"] | 0.0f;
  out.sequence = document["seq"] | 0UL;
  if (!isfinite(out.voltage) || !isfinite(out.current) || !isfinite(out.power) ||
      !isfinite(out.energy) || !isfinite(out.temperature) ||
      !isfinite(out.dailyEnergy) || !isfinite(out.weeklyEnergy) ||
      !isfinite(out.monthlyEnergy) || out.voltage < 0.0f || out.voltage > 400.0f ||
      out.current < 0.0f || out.current > 200.0f || out.power < -80000.0f ||
      out.power > 80000.0f || out.energy < 0.0f || out.dailyEnergy < 0.0f ||
      out.weeklyEnergy < 0.0f || out.monthlyEnergy < 0.0f ||
      out.temperature < (strcmp(status, "FAULT") == 0 ? -127.0f : -55.0f) ||
      out.temperature > 125.0f) return false;
  snprintf(out.timestamp, sizeof(out.timestamp), "%s", timestamp);
  memcpy(out.status, status, statusLength + 1);
  return true;
}

bool UartReceiver::hasValidPacket() const {
  return validPacket;
}
