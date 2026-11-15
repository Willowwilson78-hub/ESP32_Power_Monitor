#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

struct SensorSnapshot {
  float voltage;
  float current;
  float temperatureC;
};

class SensorManager {
public:
  SensorManager();
  void begin();
  void update();
  void sampleElectrical(float& voltageAdc, float& currentAdc);
  float readVoltageRms() const;
  float readCurrentRms() const;
  float readTemperatureC() const;
  SensorSnapshot snapshot() const;

private:
  OneWire oneWireBus;
  DallasTemperature temperatureSensor;
  float voltageSample;
  float currentSample;
  float temperatureSample;
  uint32_t temperatureRequestMs;
  bool temperatureConversionPending;
};

#endif
