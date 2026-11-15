#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <Arduino.h>
#include "time_service.h"

struct DailySummary {
  char date[11];
  float avgVoltage;
  float avgCurrent;
  float avgPower;
  float avgTemperature;
  float energyKwh;
  uint32_t timestampUnix;
};

struct WeeklySummary {
  char startDate[11];
  char endDate[11];
  float totalEnergyKwh;
  float avgPower;
  uint16_t dayCount;
  uint32_t timestampUnix;
};

struct MonthlySummary {
  char month[8];
  float totalEnergyKwh;
  float avgPower;
  uint8_t dayCount;
  uint32_t timestampUnix;
};

struct AnalyticsCheckpoint {
  uint32_t magic;
  CalendarTime activeTimestamp;
  uint32_t activeDateKey;
  uint32_t activeWeekKey;
  uint16_t activeMonthKey;
  char activeWeekStartDate[11];
  char activeMonthText[8];
  float dayStartEnergyKwh;
  float weekStartEnergyKwh;
  float monthStartEnergyKwh;
  float voltageAccumulator;
  float currentAccumulator;
  float powerAccumulator;
  float temperatureAccumulator;
  uint32_t sampleCount;
  float weeklyEnergyFromDays;
  float monthlyEnergyFromDays;
  float weeklyPowerFromDays;
  float monthlyPowerFromDays;
  uint16_t weeklyDayCount;
  uint8_t monthlyDayCount;
};

class AnalyticsEngine {
public:
  AnalyticsEngine();
  void begin();
  void update(float voltage, float current, float power, float temperature,
              float cumulativeEnergyKwh, const CalendarTime& timestamp);
  AnalyticsCheckpoint makeCheckpoint() const;
  bool restoreCheckpoint(const AnalyticsCheckpoint& checkpoint);
  bool takeDailySummary(DailySummary& summary);
  bool takeWeeklySummary(WeeklySummary& summary);
  bool takeMonthlySummary(MonthlySummary& summary);
  float getDailyEnergyKwh(float cumulativeEnergyKwh) const;
  float getWeeklyEnergyKwh(float cumulativeEnergyKwh) const;
  float getMonthlyEnergyKwh(float cumulativeEnergyKwh) const;

private:
  bool closeDay(float cumulativeEnergyKwh, const CalendarTime& timestamp);
  void closeWeek(const CalendarTime& timestamp);
  void closeMonth(const CalendarTime& timestamp);
  void addDailyToPeriods(const DailySummary& summary);

  DailySummary pendingDailySummary;
  WeeklySummary pendingWeeklySummary;
  MonthlySummary pendingMonthlySummary;
  CalendarTime activeTimestamp;
  uint32_t activeDateKey;
  uint32_t activeWeekKey;
  uint16_t activeMonthKey;
  char activeWeekStartDate[11];
  char activeMonthText[8];
  float dayStartEnergyKwh;
  float weekStartEnergyKwh;
  float monthStartEnergyKwh;
  float voltageAccumulator;
  float currentAccumulator;
  float powerAccumulator;
  float temperatureAccumulator;
  uint32_t sampleCount;
  float weeklyEnergyFromDays;
  float monthlyEnergyFromDays;
  float weeklyPowerFromDays;
  float monthlyPowerFromDays;
  uint16_t weeklyDayCount;
  uint8_t monthlyDayCount;
  bool initialized;
  bool hasDailySummary;
  bool hasWeeklySummary;
  bool hasMonthlySummary;
};

#endif
