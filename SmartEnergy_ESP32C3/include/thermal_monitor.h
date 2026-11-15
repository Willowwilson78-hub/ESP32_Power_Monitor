#ifndef THERMAL_MONITOR_H
#define THERMAL_MONITOR_H

#include <Arduino.h>

enum class ThermalState : uint8_t {
  NORMAL = 0,
  WARM = 1,
  HOT = 2,
  SENSOR_FAULT = 3
};

struct ThermalConfig {
  float warnC;
  float hotC;
};

class ThermalMonitor {
public:
  ThermalMonitor();
  void begin(float warnC = 45.0f, float hotC = 60.0f);
  void update(float temperatureC);
  ThermalState getState() const;
  float getTemperatureC() const;
  bool isHot() const;

private:
  ThermalConfig config;
  ThermalState state;
  float temperatureC;
};

#endif
