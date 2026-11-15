#include <Arduino.h>
#include <Preferences.h>
#include <stddef.h>

#include "sensor_manager.h"
#include "power_calc.h"
#include "thermal_monitor.h"
#include "display_manager.h"
#include "graph_manager.h"
#include "logger.h"
#include "analytics.h"
#include "telemetry.h"
#include "time_service.h"

#ifndef SMART_ENERGY_INITIAL_UNIX
#define SMART_ENERGY_INITIAL_UNIX 0UL
#endif

namespace {
struct PersistentState {
  uint32_t magic;
  float totalEnergyKwh;
  AnalyticsCheckpoint analytics;
  uint32_t checksum;
};

constexpr uint32_t kPersistentStateMagic = 0x454E4759UL;

uint32_t calculatePersistentChecksum(const PersistentState& state) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&state);
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < offsetof(PersistentState, checksum); ++i) {
    crc ^= bytes[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
  }
  return ~crc;
}

constexpr uint8_t kVoltageAdcPin = 2;
constexpr uint8_t kCurrentAdcPin = 3;
constexpr uint32_t kAdcSampleIntervalUs = 500;
constexpr float kVoltageCalibrationVPerCount = 0.0886f;
constexpr float kCurrentCalibrationAPerCount = 0.00806f;
constexpr uint32_t kSensorIntervalMs = 20;
constexpr uint32_t kPowerIntervalMs = 200;
constexpr uint32_t kDisplayIntervalMs = 250;
constexpr uint32_t kGraphIntervalMs = 150;
constexpr uint32_t kLogIntervalMs = 5000;
constexpr uint32_t kTelemetryIntervalMs = 1000;
constexpr uint32_t kClockIntervalMs = 1000;
constexpr uint32_t kPageIntervalMs = 15000;
constexpr uint32_t kCheckpointIntervalMs = 1800000;

SensorManager sensorManager;
PowerCalc powerCalc;
ThermalMonitor thermalMonitor;
DisplayManager displayManager;
GraphManager graphManager;
Logger logger;
AnalyticsEngine analytics;
TelemetryProducer telemetryProducer;
TimeService timeService;
Preferences preferences;

uint32_t lastSensorMs = 0;
uint32_t lastAdcUs = 0;
uint32_t lastPowerMs = 0;
uint32_t lastDisplayMs = 0;
uint32_t lastGraphMs = 0;
uint32_t lastLogMs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastClockMs = 0;
uint32_t lastPageMs = 0;
uint32_t lastCheckpointMs = 0;
uint8_t pageIndex = 0;
CalendarTime currentTime{};
bool currentTimeValid = false;

float latestVoltage = 0.0f;
float latestCurrent = 0.0f;
float latestPower = 0.0f;
float latestEnergy = 0.0f;
float latestTemperature = 0.0f;
ThermalState latestState = ThermalState::NORMAL;
}

void setup() {
  Serial.begin(115200);
  pinMode(kVoltageAdcPin, INPUT);
  pinMode(kCurrentAdcPin, INPUT);
  analogReadResolution(12);

  sensorManager.begin();
  powerCalc.begin(2000.0f, kVoltageCalibrationVPerCount,
                  kCurrentCalibrationAPerCount, 230.0f, 5.0f);
  thermalMonitor.begin(45.0f, 60.0f);
  displayManager.begin();
  graphManager.begin(displayManager);
  logger.begin();
  telemetryProducer.begin(Serial1);
  analytics.begin();
  preferences.begin("energy", false);
  PersistentState persistentState{};
  if (preferences.getBytesLength("state") == sizeof(persistentState) &&
      preferences.getBytes("state", &persistentState, sizeof(persistentState)) == sizeof(persistentState) &&
      persistentState.magic == kPersistentStateMagic &&
      persistentState.checksum == calculatePersistentChecksum(persistentState)) {
    powerCalc.restoreEnergyKwh(persistentState.totalEnergyKwh);
    analytics.restoreCheckpoint(persistentState.analytics);
  }
  if (!timeService.begin()) {
    if (SMART_ENERGY_INITIAL_UNIX >= 1704067200UL &&
        timeService.setUnixTime(SMART_ENERGY_INITIAL_UNIX)) {
      timeService.getNow(currentTime);
      currentTimeValid = currentTime.valid;
    } else {
      Serial.println("RTC time invalid; set SMART_ENERGY_INITIAL_UNIX once during commissioning.");
    }
  }

  latestState = ThermalState::NORMAL;
  Serial.println("Smart Energy Monitor booted.");
}

void loop() {
  const uint32_t now = millis();
  const uint32_t nowUs = micros();
  telemetryProducer.service();

  if (nowUs - lastAdcUs >= kAdcSampleIntervalUs) {
    float voltageAdc = 0.0f;
    float currentAdc = 0.0f;
    sensorManager.sampleElectrical(voltageAdc, currentAdc);
    powerCalc.addSample(voltageAdc, currentAdc);
    lastAdcUs = nowUs;
  }

  if (now - lastSensorMs >= kSensorIntervalMs) {
    sensorManager.update();
    latestTemperature = sensorManager.readTemperatureC();
    lastSensorMs = now;
  }

  if (now - lastClockMs >= kClockIntervalMs) {
    currentTimeValid = timeService.getNow(currentTime);
    lastClockMs = now;
  }

  if (now - lastPowerMs >= kPowerIntervalMs) {
    powerCalc.updateRms(kPowerIntervalMs);

    const ElectricalReading reading = powerCalc.getReading();
    latestVoltage = reading.voltageRms;
    latestCurrent = reading.currentRms;
    latestPower = reading.realPower;
    latestEnergy = reading.energyKwh;
    thermalMonitor.update(latestTemperature);
    latestState = thermalMonitor.getState();

    if (currentTimeValid && latestTemperature >= -55.0f) {
      analytics.update(latestVoltage, latestCurrent, latestPower, latestTemperature,
                       latestEnergy, currentTime);
      DailySummary daily;
      WeeklySummary weekly;
      MonthlySummary monthly;
      if (analytics.takeDailySummary(daily)) logger.writeDailySummary(daily);
      if (analytics.takeWeeklySummary(weekly)) logger.writeWeeklySummary(weekly);
      if (analytics.takeMonthlySummary(monthly)) logger.writeMonthlySummary(monthly);
    }
    lastPowerMs = now;
  }

  if (now - lastDisplayMs >= kDisplayIntervalMs) {
    DisplayMetrics displayMetrics;
    displayMetrics.voltage = latestVoltage;
    displayMetrics.current = latestCurrent;
    displayMetrics.power = latestPower;
    displayMetrics.energy = latestEnergy;
    displayMetrics.temperature = latestTemperature;
    displayMetrics.state = latestState;
    displayManager.update(displayMetrics);
    lastDisplayMs = now;
  }

  if (now - lastGraphMs >= kGraphIntervalMs) {
    graphManager.addSample(latestPower, latestCurrent, latestTemperature);
    graphManager.render();
    lastGraphMs = now;
  }

  if (now - lastPageMs >= kPageIntervalMs) {
    pageIndex = (pageIndex + 1) % 4;
    const GraphPage page = static_cast<GraphPage>(pageIndex);
    displayManager.setGraphPage(page);
    graphManager.setPage(page);
    graphManager.render();
    lastPageMs = now;
  }

  if (now - lastLogMs >= kLogIntervalMs) {
    if (currentTimeValid) {
      MeasurementLogRecord record{};
      snprintf(record.timestamp, sizeof(record.timestamp), "%s", currentTime.timestamp);
      snprintf(record.date, sizeof(record.date), "%s", currentTime.date);
      record.voltage = latestVoltage;
      record.current = latestCurrent;
      record.power = latestPower;
      record.energy = latestEnergy;
      record.temperature = latestTemperature;
      record.state = latestState;
      logger.writeMeasurementRecord(record);
    }
    lastLogMs = now;
  }

  if (now - lastTelemetryMs >= kTelemetryIntervalMs) {
    bool queued = false;
    {
      TelemetryPayload payload{};
      snprintf(payload.timestamp, sizeof(payload.timestamp), "%s", currentTime.timestamp);
      if (!currentTimeValid) {
        snprintf(payload.timestamp, sizeof(payload.timestamp), "UNSYNCED");
      }
      payload.voltage = latestVoltage;
      payload.current = latestCurrent;
      payload.power = latestPower;
      payload.energy = latestEnergy;
      payload.temperature = latestTemperature;
      snprintf(payload.status, sizeof(payload.status), "%s",
               latestState == ThermalState::NORMAL ? "GREEN" :
               latestState == ThermalState::WARM ? "YELLOW" :
               latestState == ThermalState::SENSOR_FAULT ? "FAULT" : "RED");
      payload.dailyEnergy = analytics.getDailyEnergyKwh(latestEnergy);
      payload.weeklyEnergy = analytics.getWeeklyEnergyKwh(latestEnergy);
      payload.monthlyEnergy = analytics.getMonthlyEnergyKwh(latestEnergy);
      queued = telemetryProducer.transmit(payload);
    }
    if (queued) lastTelemetryMs = now;
  }

  logger.service(now);

  if (now - lastCheckpointMs >= kCheckpointIntervalMs) {
    PersistentState persistentState{};
    persistentState.magic = kPersistentStateMagic;
    persistentState.totalEnergyKwh = powerCalc.getReading().energyKwh;
    persistentState.analytics = analytics.makeCheckpoint();
    persistentState.checksum = calculatePersistentChecksum(persistentState);
    preferences.putBytes("state", &persistentState, sizeof(persistentState));
    lastCheckpointMs = now;
  }
}
