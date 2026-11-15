#include "wifi_manager.h"

WifiManager::WifiManager()
    : ssid(nullptr),
      password(nullptr),
      status(WifiStatus::DISCONNECTED),
      reconnectIntervalMs(5000),
      lastConnectAttemptMs(0),
      lastConnectedMs(0),
      failedAttempts(0),
      wasConnected(false) {
}

void WifiManager::begin(const char* wifiSsid, const char* wifiPassword) {
  ssid = wifiSsid;
  password = wifiPassword;
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  status = WifiStatus::DISCONNECTED;
}

void WifiManager::tick() {
  if (ssid == nullptr || password == nullptr) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    status = WifiStatus::CONNECTED;
    if (!wasConnected) {
      lastConnectedMs = millis();
      failedAttempts = 0;
      reconnectIntervalMs = 5000;
    }
    wasConnected = true;
    return;
  }

  if (wasConnected || status == WifiStatus::CONNECTED || status == WifiStatus::DEGRADATED) {
    status = WifiStatus::RECONNECTING;
    wasConnected = false;
  }

  if (millis() - lastConnectAttemptMs >= reconnectIntervalMs) {
    status = WifiStatus::CONNECTING;
    lastConnectAttemptMs = millis();
    WiFi.begin(ssid, password);
    if (failedAttempts < 5) ++failedAttempts;
    reconnectIntervalMs = 5000UL << failedAttempts;
    if (reconnectIntervalMs > 60000UL) reconnectIntervalMs = 60000UL;
  }
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

WifiStatus WifiManager::getStatus() const {
  return status;
}

void WifiManager::setReconnectInterval(uint32_t ms) {
  reconnectIntervalMs = ms;
}
