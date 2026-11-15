#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

enum class WifiStatus : uint8_t {
  DISCONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2,
  RECONNECTING = 3,
  DEGRADATED = 4
};

class WifiManager {
public:
  WifiManager();
  void begin(const char* ssid, const char* password);
  void tick();
  bool isConnected() const;
  WifiStatus getStatus() const;
  void setReconnectInterval(uint32_t ms);

private:
  const char* ssid;
  const char* password;
  WifiStatus status;
  uint32_t reconnectIntervalMs;
  uint32_t lastConnectAttemptMs;
  uint32_t lastConnectedMs;
  uint8_t failedAttempts;
  bool wasConnected;
};

#endif
