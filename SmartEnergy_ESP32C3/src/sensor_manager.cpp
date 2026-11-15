#include "sensor_manager.h"

#include <math.h>

SensorManager::SensorManager()
    : oneWireBus(4),
      temperatureSensor(&oneWireBus),
      voltageSample(0.0f),
      currentSample(0.0f),
      temperatureSample(-127.0f),
      temperatureRequestMs(0),
      temperatureConversionPending(false) {
}

void SensorManager::begin() {
  temperatureSensor.begin();
  temperatureSensor.setResolution(10);
  temperatureSensor.setWaitForConversion(false);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  temperatureSensor.requestTemperatures();
  temperatureRequestMs = millis();
  temperatureConversionPending = true;
}

void SensorManager::update() {
  if (temperatureConversionPending && millis() - temperatureRequestMs >= 190) {
    const float reading = temperatureSensor.getTempCByIndex(0);
    if (reading != DEVICE_DISCONNECTED_C && isfinite(reading) && reading >= -55.0f && reading <= 125.0f) {
      temperatureSample = reading;
    }
    temperatureSensor.requestTemperatures();
    temperatureRequestMs = millis();
  }
}

void SensorManager::sampleElectrical(float& voltageAdc, float& currentAdc) {
  voltageSample = static_cast<float>(analogRead(2));
  currentSample = static_cast<float>(analogRead(3));
  voltageAdc = voltageSample;
  currentAdc = currentSample;
}

float SensorManager::readVoltageRms() const {
  return voltageSample;
}

float SensorManager::readCurrentRms() const {
  return currentSample;
}

float SensorManager::readTemperatureC() const {
  return temperatureSample;
}

SensorSnapshot SensorManager::snapshot() const {
  SensorSnapshot snapshot;
  snapshot.voltage = voltageSample;
  snapshot.current = currentSample;
  snapshot.temperatureC = temperatureSample;
  return snapshot;
}
