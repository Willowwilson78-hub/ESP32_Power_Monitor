#ifndef UART_RECEIVER_H
#define UART_RECEIVER_H

#include <Arduino.h>

struct UartTelemetry {
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

class UartReceiver {
public:
  UartReceiver();
  void begin(HardwareSerial& serialPort);
  bool receivePacket();
  const char* getLastPacket() const;
  bool parseTelemetry(const char* json, UartTelemetry& out);
  bool hasValidPacket() const;

private:
  HardwareSerial* serialPort;
  char rxBuffer[256];
  size_t rxIndex;
  bool validPacket;
  bool discardingFrame;
};

#endif
