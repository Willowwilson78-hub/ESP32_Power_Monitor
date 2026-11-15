#include "analytics.h"

#include <math.h>
#include <string.h>

namespace {
constexpr uint32_t kCheckpointMagic = 0x53454E47UL;
}

AnalyticsCheckpoint AnalyticsEngine::makeCheckpoint() const {
  AnalyticsCheckpoint checkpoint{};
  checkpoint.magic = kCheckpointMagic;
  checkpoint.activeTimestamp = activeTimestamp;
  checkpoint.activeDateKey = activeDateKey;
  checkpoint.activeWeekKey = activeWeekKey;
  checkpoint.activeMonthKey = activeMonthKey;
  memcpy(checkpoint.activeWeekStartDate, activeWeekStartDate, sizeof(activeWeekStartDate));
  memcpy(checkpoint.activeMonthText, activeMonthText, sizeof(activeMonthText));
  checkpoint.dayStartEnergyKwh = dayStartEnergyKwh;
  checkpoint.weekStartEnergyKwh = weekStartEnergyKwh;
  checkpoint.monthStartEnergyKwh = monthStartEnergyKwh;
  checkpoint.voltageAccumulator = voltageAccumulator;
  checkpoint.currentAccumulator = currentAccumulator;
  checkpoint.powerAccumulator = powerAccumulator;
  checkpoint.temperatureAccumulator = temperatureAccumulator;
  checkpoint.sampleCount = sampleCount;
  checkpoint.weeklyEnergyFromDays = weeklyEnergyFromDays;
  checkpoint.monthlyEnergyFromDays = monthlyEnergyFromDays;
  checkpoint.weeklyPowerFromDays = weeklyPowerFromDays;
  checkpoint.monthlyPowerFromDays = monthlyPowerFromDays;
  checkpoint.weeklyDayCount = weeklyDayCount;
  checkpoint.monthlyDayCount = monthlyDayCount;
  return checkpoint;
}

bool AnalyticsEngine::restoreCheckpoint(const AnalyticsCheckpoint& checkpoint) {
  if (checkpoint.magic != kCheckpointMagic || !checkpoint.activeTimestamp.valid ||
      checkpoint.sampleCount > 10000000UL || checkpoint.weeklyDayCount > 7 ||
  checkpoint.monthlyDayCount > 31 || !isfinite(checkpoint.dayStartEnergyKwh) ||
  !isfinite(checkpoint.weekStartEnergyKwh) || !isfinite(checkpoint.monthStartEnergyKwh) ||
  !isfinite(checkpoint.voltageAccumulator) || !isfinite(checkpoint.currentAccumulator) ||
  !isfinite(checkpoint.powerAccumulator) || !isfinite(checkpoint.temperatureAccumulator) ||
  !isfinite(checkpoint.weeklyEnergyFromDays) || !isfinite(checkpoint.monthlyEnergyFromDays) ||
  !isfinite(checkpoint.weeklyPowerFromDays) || !isfinite(checkpoint.monthlyPowerFromDays)) return false;
  activeTimestamp = checkpoint.activeTimestamp;
  activeDateKey = checkpoint.activeDateKey;
  activeWeekKey = checkpoint.activeWeekKey;
  activeMonthKey = checkpoint.activeMonthKey;
  memcpy(activeWeekStartDate, checkpoint.activeWeekStartDate, sizeof(activeWeekStartDate));
  memcpy(activeMonthText, checkpoint.activeMonthText, sizeof(activeMonthText));
  dayStartEnergyKwh = checkpoint.dayStartEnergyKwh;
  weekStartEnergyKwh = checkpoint.weekStartEnergyKwh;
  monthStartEnergyKwh = checkpoint.monthStartEnergyKwh;
  voltageAccumulator = checkpoint.voltageAccumulator;
  currentAccumulator = checkpoint.currentAccumulator;
  powerAccumulator = checkpoint.powerAccumulator;
  temperatureAccumulator = checkpoint.temperatureAccumulator;
  sampleCount = checkpoint.sampleCount;
  weeklyEnergyFromDays = checkpoint.weeklyEnergyFromDays;
  monthlyEnergyFromDays = checkpoint.monthlyEnergyFromDays;
  weeklyPowerFromDays = checkpoint.weeklyPowerFromDays;
  monthlyPowerFromDays = checkpoint.monthlyPowerFromDays;
  weeklyDayCount = checkpoint.weeklyDayCount;
  monthlyDayCount = checkpoint.monthlyDayCount;
  initialized = true;
  return true;
}

AnalyticsEngine::AnalyticsEngine()
    : activeTimestamp{},
      activeDateKey(0),
      activeWeekKey(0),
      activeMonthKey(0),
      activeWeekStartDate{0},
      activeMonthText{0},
      dayStartEnergyKwh(0.0f),
      weekStartEnergyKwh(0.0f),
      monthStartEnergyKwh(0.0f),
      voltageAccumulator(0.0f),
      currentAccumulator(0.0f),
      powerAccumulator(0.0f),
      temperatureAccumulator(0.0f),
      sampleCount(0),
      weeklyEnergyFromDays(0.0f),
      monthlyEnergyFromDays(0.0f),
      weeklyPowerFromDays(0.0f),
      monthlyPowerFromDays(0.0f),
      weeklyDayCount(0),
      monthlyDayCount(0),
      initialized(false),
      hasDailySummary(false),
      hasWeeklySummary(false),
      hasMonthlySummary(false) {
}

void AnalyticsEngine::begin() {
  voltageAccumulator = 0.0f;
  currentAccumulator = 0.0f;
  powerAccumulator = 0.0f;
  temperatureAccumulator = 0.0f;
  sampleCount = 0;
  weeklyEnergyFromDays = 0.0f;
  monthlyEnergyFromDays = 0.0f;
  weeklyPowerFromDays = 0.0f;
  monthlyPowerFromDays = 0.0f;
  weeklyDayCount = 0;
  monthlyDayCount = 0;
  initialized = false;
  hasDailySummary = false;
  hasWeeklySummary = false;
  hasMonthlySummary = false;
}

void AnalyticsEngine::update(float voltage, float current, float power, float temperature,
                            float cumulativeEnergyKwh, const CalendarTime& timestamp) {
  if (!timestamp.valid) {
    return;
  }
  if (!initialized) {
    activeTimestamp = timestamp;
    activeDateKey = timestamp.dateKey;
    activeWeekKey = timestamp.weekKey;
    activeMonthKey = timestamp.monthKey;
    snprintf(activeWeekStartDate, sizeof(activeWeekStartDate), "%s", timestamp.date);
    snprintf(activeMonthText, sizeof(activeMonthText), "%s", timestamp.monthText);
    dayStartEnergyKwh = cumulativeEnergyKwh;
    weekStartEnergyKwh = cumulativeEnergyKwh;
    monthStartEnergyKwh = cumulativeEnergyKwh;
    initialized = true;
  } else if (timestamp.dateKey != activeDateKey) {
    if (closeDay(cumulativeEnergyKwh, activeTimestamp)) {
      addDailyToPeriods(pendingDailySummary);
    }

    if (timestamp.weekKey != activeWeekKey) {
      closeWeek(activeTimestamp);
      weeklyEnergyFromDays = 0.0f;
      weeklyPowerFromDays = 0.0f;
      weeklyDayCount = 0;
      weekStartEnergyKwh = cumulativeEnergyKwh;
      activeWeekKey = timestamp.weekKey;
      snprintf(activeWeekStartDate, sizeof(activeWeekStartDate), "%s", timestamp.date);
    }
    if (timestamp.monthKey != activeMonthKey) {
      closeMonth(activeTimestamp);
      monthlyEnergyFromDays = 0.0f;
      monthlyPowerFromDays = 0.0f;
      monthlyDayCount = 0;
      monthStartEnergyKwh = cumulativeEnergyKwh;
      activeMonthKey = timestamp.monthKey;
      snprintf(activeMonthText, sizeof(activeMonthText), "%s", timestamp.monthText);
    }

    activeDateKey = timestamp.dateKey;
    dayStartEnergyKwh = cumulativeEnergyKwh;
    voltageAccumulator = 0.0f;
    currentAccumulator = 0.0f;
    powerAccumulator = 0.0f;
    temperatureAccumulator = 0.0f;
    sampleCount = 0;
  }

  activeTimestamp = timestamp;
  voltageAccumulator += voltage;
  currentAccumulator += current;
  powerAccumulator += power;
  temperatureAccumulator += temperature;
  ++sampleCount;
}

bool AnalyticsEngine::closeDay(float cumulativeEnergyKwh, const CalendarTime& timestamp) {
  if (sampleCount == 0) {
    return false;
  }

  snprintf(pendingDailySummary.date, sizeof(pendingDailySummary.date), "%s", timestamp.date);
  pendingDailySummary.avgVoltage = voltageAccumulator / static_cast<float>(sampleCount);
  pendingDailySummary.avgCurrent = currentAccumulator / static_cast<float>(sampleCount);
  pendingDailySummary.avgPower = powerAccumulator / static_cast<float>(sampleCount);
  pendingDailySummary.avgTemperature = temperatureAccumulator / static_cast<float>(sampleCount);
  pendingDailySummary.energyKwh = max(0.0f, cumulativeEnergyKwh - dayStartEnergyKwh);
  pendingDailySummary.timestampUnix = timestamp.unixTime;
  hasDailySummary = true;
  return true;
}

void AnalyticsEngine::addDailyToPeriods(const DailySummary& summary) {
  weeklyEnergyFromDays += summary.energyKwh;
  monthlyEnergyFromDays += summary.energyKwh;
  weeklyPowerFromDays += summary.avgPower;
  monthlyPowerFromDays += summary.avgPower;
  ++weeklyDayCount;
  ++monthlyDayCount;
}

void AnalyticsEngine::closeWeek(const CalendarTime& timestamp) {
  if (weeklyDayCount == 0) {
    return;
  }
  snprintf(pendingWeeklySummary.startDate, sizeof(pendingWeeklySummary.startDate), "%s", activeWeekStartDate);
  snprintf(pendingWeeklySummary.endDate, sizeof(pendingWeeklySummary.endDate), "%s", timestamp.date);
  pendingWeeklySummary.totalEnergyKwh = weeklyEnergyFromDays;
  pendingWeeklySummary.avgPower = weeklyPowerFromDays / weeklyDayCount;
  pendingWeeklySummary.dayCount = weeklyDayCount;
  pendingWeeklySummary.timestampUnix = timestamp.unixTime;
  hasWeeklySummary = true;
}

void AnalyticsEngine::closeMonth(const CalendarTime& timestamp) {
  if (monthlyDayCount == 0) {
    return;
  }
  snprintf(pendingMonthlySummary.month, sizeof(pendingMonthlySummary.month), "%s", activeMonthText);
  pendingMonthlySummary.totalEnergyKwh = monthlyEnergyFromDays;
  pendingMonthlySummary.avgPower = monthlyPowerFromDays / monthlyDayCount;
  pendingMonthlySummary.dayCount = monthlyDayCount;
  pendingMonthlySummary.timestampUnix = timestamp.unixTime;
  hasMonthlySummary = true;
}

bool AnalyticsEngine::takeDailySummary(DailySummary& summary) {
  if (!hasDailySummary) return false;
  summary = pendingDailySummary;
  hasDailySummary = false;
  return true;
}

bool AnalyticsEngine::takeWeeklySummary(WeeklySummary& summary) {
  if (!hasWeeklySummary) return false;
  summary = pendingWeeklySummary;
  hasWeeklySummary = false;
  return true;
}

bool AnalyticsEngine::takeMonthlySummary(MonthlySummary& summary) {
  if (!hasMonthlySummary) return false;
  summary = pendingMonthlySummary;
  hasMonthlySummary = false;
  return true;
}

float AnalyticsEngine::getDailyEnergyKwh(float cumulativeEnergyKwh) const {
  return initialized ? max(0.0f, cumulativeEnergyKwh - dayStartEnergyKwh) : 0.0f;
}

float AnalyticsEngine::getWeeklyEnergyKwh(float cumulativeEnergyKwh) const {
  return initialized ? max(0.0f, cumulativeEnergyKwh - weekStartEnergyKwh) : 0.0f;
}

float AnalyticsEngine::getMonthlyEnergyKwh(float cumulativeEnergyKwh) const {
  return initialized ? max(0.0f, cumulativeEnergyKwh - monthStartEnergyKwh) : 0.0f;
}
