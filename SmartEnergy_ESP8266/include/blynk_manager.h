#ifndef BLYNK_MANAGER_H
#define BLYNK_MANAGER_H

#include <Arduino.h>
#include "uart_receiver.h"

class BlynkManager {
public:
  BlynkManager();
  void begin(const char* authToken);
  bool loop();
  bool uploadTelemetry(const UartTelemetry& telemetry);
  bool isReady() const;

private:
  const char* authToken;
  bool initialized;
  uint32_t lastUploadMs;
  uint32_t lastConnectAttemptMs;
};

#endif
