# ESP8266 Smart Energy Gateway

This project implements the communication gateway and cloud integration side of the Smart Energy Monitoring architecture.

## Responsibilities

- UART telemetry receiver
- JSON parsing and validation
- WiFi manager and reconnect logic
- offline packet buffering in fixed-size ring buffer
- Blynk virtual pin uploads
- cloud synchronization with FIFO replay when restored

## Main files

- src/main.cpp
- src/uart_receiver.cpp
- src/wifi_manager.cpp
- src/blynk_manager.cpp
- src/packet_queue.cpp
