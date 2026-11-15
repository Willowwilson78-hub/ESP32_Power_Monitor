#ifndef POWER_CALC_H
#define POWER_CALC_H

#include <Arduino.h>

struct ElectricalReading {
  float voltageRms;
  float currentRms;
  float apparentPower;
  float realPower;
  float energyKwh;
  float powerFactor;
};

class PowerCalc {
public:
  PowerCalc();
  void begin(float sampleRateHz, float voltageScale, float currentScale,
             float voltageRef, float currentRef);
  void addSample(float voltageRaw, float currentRaw);
  void updateRms(uint32_t sampleWindowMs);
  ElectricalReading getReading() const;
  float accumulateEnergyKwh(float powerWatts, uint32_t deltaMs);
  void restoreEnergyKwh(float energyKwh);

private:
  static constexpr size_t kSampleWindow = 256;

  float voltageSamples[kSampleWindow];
  float currentSamples[kSampleWindow];
  uint16_t sampleCount;
  uint16_t nextSampleIndex;
  float voltageScale;
  float currentScale;
  float voltageReference;
  float currentReference;
  float cumulativeEnergyKwh;
  uint32_t lastEnergyTimestampMs;
  ElectricalReading lastReading;
};

#endif
