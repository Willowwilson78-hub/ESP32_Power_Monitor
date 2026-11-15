# ESP32-C3 Smart Energy Monitor

This project implements the measurement, analytics, display, storage, and telemetry generation side of the Smart Energy Monitoring architecture.

## Responsibilities

- ADC acquisition from current and voltage sensors
- RMS calculations for voltage and current
- apparent and real power estimation
- energy accumulation
- DS18B20 temperature monitoring
- thermal threshold evaluation
- ST7735 TFT dashboard and graph rendering
- CSV logging to MicroSD
- telemetry serialization and UART Tx

## Commissioning and calibration

- Install a DS1302 RTC module with its backup battery. If its time is unset, build once with `SMART_ENERGY_INITIAL_UNIX` set to the current Unix epoch, then remove that commissioning value. The RTC retains time across ESP32-C3 resets thanks to its backup battery.
- Calibrate `kVoltageCalibrationVPerCount` and `kCurrentCalibrationAPerCount` in `src/main.cpp` against a known-safe isolated reference before trusting readings. Values in the source are starting points, not certified calibration constants.
- The analog front ends must bias AC waveforms within the ESP32-C3 ADC input range and provide galvanic isolation appropriate to mains measurement.

## ESP32-C3 Super Mini wiring

| Signal | GPIO / pin |
| --- | --- |
| TFT CS / DC | GPIO0 / GPIO1 |
| Voltage / current ADC | GPIO2 / GPIO3 |
| DS18B20 DQ | GPIO4, 4.7 kOhm pull-up to 3V3 |
| SD MISO / shared SCK / shared MOSI | GPIO5 / GPIO6 / GPIO7 |
| DS1302 VCC / GND | 3V3 / common ground |
| DS1302 DAT / CLK / RST (CE) | GPIO8 / GPIO9 / GPIO4 |
| SD CS | GPIO10 |
| UART RX / TX | GPIO20 / GPIO21 |

TFT and SD share SPI SCK/MOSI and use separate CS lines. TFT reset is configured as `-1` (held at 3V3); TFT backlight is also wired to 3V3. The DS1302 is a 3-wire device, not I2C; use its backup battery to retain time across resets. GPIO8 and GPIO9 are strapping pins; do not drive them low during reset. The RTC is initialized only after `setup()` starts. Power off before wiring. Connect mains only through isolated, correctly rated sensor modules; never connect mains directly to an ADC pin. GPIO2, GPIO8, and GPIO9 have boot/strapping considerations on this board, so ensure attached modules do not force invalid boot levels.

## Main files

- src/main.cpp
- src/sensor_manager.cpp
- src/power_calc.cpp
- src/thermal_monitor.cpp
- src/display_manager.cpp
- src/graph_manager.cpp
- src/logger.cpp
- src/analytics.cpp
- src/telemetry.cpp
