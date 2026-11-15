#include "blynk_manager.h"

#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>

BlynkManager::BlynkManager()
  : authToken(nullptr), initialized(false), lastUploadMs(0), lastConnectAttemptMs(0) {
}

void BlynkManager::begin(const char* token) {
  authToken = token;
  initialized = (authToken != nullptr);
  if (initialized) {
    Blynk.config(authToken);
  }
}

bool BlynkManager::loop() {
  if (!initialized) {
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) return false;
  if (Blynk.connected()) {
    Blynk.run();
    return true;
  }
  if (millis() - lastConnectAttemptMs >= 10000UL) {
    lastConnectAttemptMs = millis();
    Blynk.connect(1);
  }
  if (Blynk.connected()) Blynk.run();
  return Blynk.connected();
}

bool BlynkManager::uploadTelemetry(const UartTelemetry& telemetry) {
  if (!initialized || !Blynk.connected()) {
    return false;
  }

  Blynk.virtualWrite(V0, telemetry.voltage);
  Blynk.virtualWrite(V1, telemetry.current);
  Blynk.virtualWrite(V2, telemetry.power);
  Blynk.virtualWrite(V3, telemetry.energy);
  Blynk.virtualWrite(V4, telemetry.temperature);
  Blynk.virtualWrite(V5, telemetry.status);
  Blynk.virtualWrite(V6, telemetry.dailyEnergy);
  Blynk.virtualWrite(V7, telemetry.weeklyEnergy);
  Blynk.virtualWrite(V8, telemetry.monthlyEnergy);

  lastUploadMs = millis();
  return true;
}

bool BlynkManager::isReady() const {
  return initialized && Blynk.connected();
}
