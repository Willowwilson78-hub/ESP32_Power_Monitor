#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <SD.h>
#include "analytics.h"
#include "thermal_monitor.h"

struct MeasurementLogRecord {
  char timestamp[32];
  char date[11];
  float voltage;
  float current;
  float power;
  float energy;
  float temperature;
  ThermalState state;
};

struct SummaryLogRecord {
  char path[40];
  char header[120];
  char line[160];
};

class Logger {
public:
  static constexpr uint8_t kQueueCapacity = 16;
  static constexpr uint8_t kSummaryQueueCapacity = 4;

  Logger();
  bool begin();
  bool writeMeasurementRecord(const MeasurementLogRecord& record);
  bool writeDailySummary(const DailySummary& summary);
  bool writeWeeklySummary(const WeeklySummary& summary);
  bool writeMonthlySummary(const MonthlySummary& summary);
  void service(uint32_t nowMs);
  bool isReady() const;

private:
  bool mountCard();
  bool ensureDirectories();
  bool openRawFile(const char* date);
  bool queueSummaryLine(const char* path, const char* header, const char* line);
  bool appendSummaryLine(const char* path, const char* header, const char* line);
  void storageFailure();

  MeasurementLogRecord queue[kQueueCapacity];
  uint8_t queueHead;
  uint8_t queueTail;
  uint8_t queueCount;
  SummaryLogRecord summaryQueue[kSummaryQueueCapacity];
  uint8_t summaryHead;
  uint8_t summaryTail;
  uint8_t summaryCount;
  File rawFile;
  char openDate[11];
  bool sdReady;
  uint32_t lastRetryMs;
  uint32_t lastFlushMs;
  uint8_t recordsSinceFlush;
};

#endif
