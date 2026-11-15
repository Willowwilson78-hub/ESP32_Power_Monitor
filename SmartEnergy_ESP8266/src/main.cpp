#include <Arduino.h>

#include "wifi_manager.h"
#include "uart_receiver.h"
#include "packet_queue.h"
#include "blynk_manager.h"

namespace {
constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
constexpr char kBlynkToken[] = "YOUR_BLYNK_TOKEN";

constexpr uint32_t kQueueCheckMs = 1000;

WifiManager wifiManager;
UartReceiver uartReceiver;
PacketQueue packetQueue;
BlynkManager blynkManager;

uint32_t lastQueueCheckMs = 0;
uint32_t lastUploadMs = 0;
}

void setup() {
  uartReceiver.begin(Serial);
  Serial.swap();
  wifiManager.begin(kWifiSsid, kWifiPassword);
  blynkManager.begin(kBlynkToken);
  packetQueue.reset();

}

void loop() {
  const uint32_t now = millis();

  wifiManager.tick();
  blynkManager.loop();

  if (uartReceiver.receivePacket()) {
    const char* packet = uartReceiver.getLastPacket();
    if (packet != nullptr && strlen(packet) > 0 && !packetQueue.enqueue(packet)) {
      // Preserve FIFO order by dropping the newest frame when the queue is full.
    }
  }

  if (now - lastQueueCheckMs >= kQueueCheckMs) {
    if (wifiManager.isConnected() && !packetQueue.isEmpty()) {
      char packet[256];
      if (packetQueue.peek(packet, sizeof(packet))) {
        UartTelemetry telemetry;
        const bool valid = uartReceiver.parseTelemetry(packet, telemetry);
        if (!valid) {
          packetQueue.discardOldest();
        } else if (blynkManager.uploadTelemetry(telemetry)) {
          packetQueue.discardOldest();
        }
      }
    }
    lastQueueCheckMs = now;
  }

}
