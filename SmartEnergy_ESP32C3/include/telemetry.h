#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>
#include "thermal_monitor.h"

struct TelemetryPayload {
  char timestamp[20];
  uint32_t sequence;
  float voltage;
  float current;
  float power;
  float energy;
  float temperature;
  char status[12];
  float dailyEnergy;
  float weeklyEnergy;
  float monthlyEnergy;
};

class TelemetryProducer {
public:
  static constexpr uint8_t kTxQueueCapacity = 4;

  TelemetryProducer();
  void begin(HardwareSerial& serialPort);
  bool transmit(const TelemetryPayload& payload);
  void service();
  bool parseIncomingLine(const char* line, TelemetryPayload& payload);

private:
  HardwareSerial* serialPort;
  char txBuffer[256];
  char txQueue[kTxQueueCapacity][256];
  uint16_t txLengths[kTxQueueCapacity];
  uint16_t txOffsets[kTxQueueCapacity];
  uint8_t txHead;
  uint8_t txTail;
  uint8_t txCount;
  uint32_t nextSequence;
};

#endif
