#include "thermal_monitor.h"

#include <math.h>

ThermalMonitor::ThermalMonitor()
    : config{45.0f, 60.0f}, state(ThermalState::NORMAL), temperatureC(0.0f) {
}

void ThermalMonitor::begin(float warnC, float hotC) {
  if (!isfinite(warnC) || !isfinite(hotC) || hotC <= warnC) {
    warnC = 45.0f;
    hotC = 60.0f;
  }
  config.warnC = warnC;
  config.hotC = hotC;
  state = ThermalState::NORMAL;
}

void ThermalMonitor::update(float temperature) {
  temperatureC = temperature;

  if (!isfinite(temperatureC) || temperatureC < -55.0f || temperatureC > 125.0f) {
    state = ThermalState::SENSOR_FAULT;
    return;
  }

  if (temperatureC >= config.hotC) {
    state = ThermalState::HOT;
  } else if (temperatureC >= config.warnC) {
    state = ThermalState::WARM;
  } else {
    state = ThermalState::NORMAL;
  }
}

ThermalState ThermalMonitor::getState() const {
  return state;
}

float ThermalMonitor::getTemperatureC() const {
  return temperatureC;
}

bool ThermalMonitor::isHot() const {
  return state == ThermalState::HOT;
}
