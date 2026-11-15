#include "power_calc.h"

#include <math.h>

PowerCalc::PowerCalc() : sampleCount(0), nextSampleIndex(0),
                         voltageScale(1.0f), currentScale(1.0f),
                         voltageReference(220.0f), currentReference(5.0f),
                         cumulativeEnergyKwh(0.0f), lastEnergyTimestampMs(0),
                         lastReading{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f} {
  for (size_t i = 0; i < kSampleWindow; ++i) {
    voltageSamples[i] = 0.0f;
    currentSamples[i] = 0.0f;
  }
}

void PowerCalc::begin(float sampleRateHz, float voltageScaleValue, float currentScaleValue,
                      float voltageRef, float currentRef) {
  (void)sampleRateHz;
  voltageScale = voltageScaleValue;
  currentScale = currentScaleValue;
  voltageReference = voltageRef;
  currentReference = currentRef;
  lastEnergyTimestampMs = millis();
}

void PowerCalc::addSample(float voltageRaw, float currentRaw) {
  voltageSamples[nextSampleIndex] = voltageRaw;
  currentSamples[nextSampleIndex] = currentRaw;
  nextSampleIndex = (nextSampleIndex + 1) % kSampleWindow;
  if (sampleCount < kSampleWindow) ++sampleCount;
}

void PowerCalc::updateRms(uint32_t sampleWindowMs) {
  (void)sampleWindowMs;
  if (sampleCount == 0) {
    return;
  }

  float meanVoltageAdc = 0.0f;
  float meanCurrentAdc = 0.0f;
  const size_t validSamples = sampleCount;

  for (size_t i = 0; i < validSamples; ++i) {
    meanVoltageAdc += voltageSamples[i];
    meanCurrentAdc += currentSamples[i];
  }
  meanVoltageAdc /= static_cast<float>(validSamples);
  meanCurrentAdc /= static_cast<float>(validSamples);

  float sumSquareVoltage = 0.0f;
  float sumSquareCurrent = 0.0f;
  float sumInstantPower = 0.0f;
  for (size_t i = 0; i < validSamples; ++i) {
    const float voltageAc = voltageSamples[i] - meanVoltageAdc;
    const float currentAc = currentSamples[i] - meanCurrentAdc;
    sumSquareVoltage += voltageAc * voltageAc;
    sumSquareCurrent += currentAc * currentAc;
    sumInstantPower += voltageAc * currentAc;
  }

  const float rmsVoltage = sqrt(sumSquareVoltage / static_cast<float>(validSamples)) * voltageScale;
  const float rmsCurrent = sqrt(sumSquareCurrent / static_cast<float>(validSamples)) * currentScale;
  const float apparentPower = rmsVoltage * rmsCurrent;
  const float realPower = (sumInstantPower / static_cast<float>(validSamples)) * voltageScale * currentScale;
  const float powerFactor = apparentPower > 0.0f ? realPower / apparentPower : 0.0f;

  const uint32_t now = millis();
  cumulativeEnergyKwh += accumulateEnergyKwh(realPower, now - lastEnergyTimestampMs);
  lastEnergyTimestampMs = now;

  lastReading.voltageRms = rmsVoltage;
  lastReading.currentRms = rmsCurrent;
  lastReading.apparentPower = apparentPower;
  lastReading.realPower = realPower;
  lastReading.energyKwh = cumulativeEnergyKwh;
  lastReading.powerFactor = powerFactor;
}

float PowerCalc::accumulateEnergyKwh(float powerWatts, uint32_t deltaMs) {
  if (deltaMs == 0 || powerWatts <= 0.0f) {
    return 0.0f;
  }

  const float deltaHours = static_cast<float>(deltaMs) / 3600000.0f;
  return (powerWatts * deltaHours) / 1000.0f;
}

ElectricalReading PowerCalc::getReading() const {
  return lastReading;
}

void PowerCalc::restoreEnergyKwh(float energyKwh) {
  if (!isfinite(energyKwh) || energyKwh < 0.0f) return;
  cumulativeEnergyKwh = energyKwh;
  lastReading.energyKwh = energyKwh;
  lastEnergyTimestampMs = millis();
}
