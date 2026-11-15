#include "logger.h"

#include <SPI.h>
#include <time.h>

namespace {
constexpr uint8_t kSdChipSelectPin = 10;
constexpr uint32_t kRetryIntervalMs = 10000;
constexpr uint32_t kFlushIntervalMs = 60000;

const char* thermalStateToString(ThermalState state) {
  switch (state) {
    case ThermalState::NORMAL:
      return "GREEN";
    case ThermalState::WARM:
      return "YELLOW";
    case ThermalState::HOT:
      return "RED";
    case ThermalState::SENSOR_FAULT:
      return "FAULT";
    default:
      return "UNKNOWN";
  }
}
}

Logger::Logger()
    : queueHead(0), queueTail(0), queueCount(0), summaryHead(0), summaryTail(0),
      summaryCount(0), openDate{0}, sdReady(false),
      lastRetryMs(0), lastFlushMs(0), recordsSinceFlush(0) {}

bool Logger::begin() {
  return mountCard();
}

bool Logger::mountCard() {
  rawFile.close();
  openDate[0] = '\0';
  SPI.begin(6, 5, 7, kSdChipSelectPin);
  if (!SD.begin(kSdChipSelectPin)) {
    sdReady = false;
    return false;
  }
  sdReady = ensureDirectories();
  return sdReady;
}

bool Logger::ensureDirectories() {
  if (!SD.exists("/data")) {
    if (!SD.mkdir("/data")) {
      return false;
    }
  }

  if (!SD.exists("/data/raw")) {
    if (!SD.mkdir("/data/raw")) {
      return false;
    }
  }

  if (!SD.exists("/data/summary")) {
    if (!SD.mkdir("/data/summary")) {
      return false;
    }
  }

  return true;
}

bool Logger::writeMeasurementRecord(const MeasurementLogRecord& record) {
  if (queueCount >= kQueueCapacity || record.date[0] == '\0') {
    return false;
  }

  queue[queueHead] = record;
  queue[queueHead].timestamp[sizeof(queue[queueHead].timestamp) - 1] = '\0';
  queue[queueHead].date[sizeof(queue[queueHead].date) - 1] = '\0';
  queueHead = (queueHead + 1) % kQueueCapacity;
  ++queueCount;
  return true;
}

bool Logger::writeDailySummary(const DailySummary& summary) {
  char line[160];
  snprintf(line, sizeof(line), "%s,%u,%.2f,%.2f,%.2f,%.2f,%.4f\n",
           summary.date, summary.timestampUnix, summary.avgVoltage,
           summary.avgCurrent, summary.avgPower, summary.avgTemperature,
           summary.energyKwh);
  return queueSummaryLine("/data/summary/daily_summary.csv",
      "Date,TimestampUnix,AverageVoltage,AverageCurrent,AveragePower,AverageTemperature,EnergyKWh\n", line);
}

bool Logger::writeWeeklySummary(const WeeklySummary& summary) {
  char line[128];
  snprintf(line, sizeof(line), "%s,%s,%u,%u,%.4f,%.2f\n",
           summary.startDate, summary.endDate, summary.dayCount,
           summary.timestampUnix, summary.totalEnergyKwh, summary.avgPower);
  return queueSummaryLine("/data/summary/weekly_summary.csv",
      "WeekStart,WeekEnd,Days,TimestampUnix,EnergyKWh,AveragePower\n", line);
}

bool Logger::writeMonthlySummary(const MonthlySummary& summary) {
  char line[128];
  snprintf(line, sizeof(line), "%s,%u,%u,%.4f,%.2f\n", summary.month,
           summary.dayCount, summary.timestampUnix, summary.totalEnergyKwh,
           summary.avgPower);
  return queueSummaryLine("/data/summary/monthly_summary.csv",
      "Month,Days,TimestampUnix,EnergyKWh,AveragePower\n", line);
}

bool Logger::queueSummaryLine(const char* path, const char* header, const char* line) {
  if (summaryCount >= kSummaryQueueCapacity) return false;
  SummaryLogRecord& record = summaryQueue[summaryHead];
  const int pathLength = snprintf(record.path, sizeof(record.path), "%s", path);
  const int headerLength = snprintf(record.header, sizeof(record.header), "%s", header);
  const int lineLength = snprintf(record.line, sizeof(record.line), "%s", line);
  if (pathLength <= 0 || pathLength >= static_cast<int>(sizeof(record.path)) ||
      headerLength <= 0 || headerLength >= static_cast<int>(sizeof(record.header)) ||
      lineLength <= 0 || lineLength >= static_cast<int>(sizeof(record.line))) return false;
  summaryHead = (summaryHead + 1) % kSummaryQueueCapacity;
  ++summaryCount;
  return true;
}

bool Logger::appendSummaryLine(const char* path, const char* header, const char* line) {
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    storageFailure();
    return false;
  }

  if (file.size() == 0) {
    if (file.print(header) != strlen(header)) {
      file.close();
      storageFailure();
      return false;
    }
  }

  const bool success = file.print(line) == strlen(line);
  if (success) {
    file.flush();
  }
  file.close();
  if (!success) storageFailure();
  return success;
}

bool Logger::openRawFile(const char* date) {
  char path[40];
  snprintf(path, sizeof(path), "/data/raw/%s.csv", date);
  rawFile = SD.open(path, FILE_WRITE);
  if (!rawFile) {
    storageFailure();
    return false;
  }
  if (rawFile.size() == 0) {
    static const char header[] = "Timestamp,Voltage,Current,Power,Energy,Temperature,Status\n";
    if (rawFile.print(header) != sizeof(header) - 1) {
      storageFailure();
      return false;
    }
  }
  snprintf(openDate, sizeof(openDate), "%s", date);
  return true;
}

void Logger::storageFailure() {
  rawFile.close();
  openDate[0] = '\0';
  SD.end();
  sdReady = false;
}

void Logger::service(uint32_t nowMs) {
  if (!sdReady) {
    if (static_cast<uint32_t>(nowMs - lastRetryMs) < kRetryIntervalMs) return;
    lastRetryMs = nowMs;
    mountCard();
    return;
  }

  if (summaryCount > 0) {
    const SummaryLogRecord& summary = summaryQueue[summaryTail];
    rawFile.close();
    openDate[0] = '\0';
    if (!appendSummaryLine(summary.path, summary.header, summary.line)) return;
    summaryTail = (summaryTail + 1) % kSummaryQueueCapacity;
    --summaryCount;
    return;
  }

  if (queueCount > 0) {
    const MeasurementLogRecord& record = queue[queueTail];
    if (strcmp(openDate, record.date) != 0) {
      rawFile.close();
      openDate[0] = '\0';
      if (!openRawFile(record.date)) return;
    }

    char line[160];
    snprintf(line, sizeof(line), "%s,%.2f,%.2f,%.0f,%.4f,%.1f,%s\n",
             record.timestamp, record.voltage, record.current, record.power,
             record.energy, record.temperature, thermalStateToString(record.state));
    if (rawFile.print(line) != strlen(line)) {
      storageFailure();
      return;
    }
    queueTail = (queueTail + 1) % kQueueCapacity;
    --queueCount;
    ++recordsSinceFlush;
  }

  if (rawFile && (recordsSinceFlush >= 12 || nowMs - lastFlushMs >= kFlushIntervalMs)) {
    rawFile.flush();
    lastFlushMs = nowMs;
    recordsSinceFlush = 0;
  }
}

bool Logger::isReady() const {
  return sdReady;
}
