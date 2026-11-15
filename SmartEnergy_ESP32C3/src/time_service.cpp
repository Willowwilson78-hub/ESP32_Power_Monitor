#include "time_service.h"

#include <stdlib.h>
#include <time.h>

namespace {
constexpr uint8_t kRtcClkPin = 9;
constexpr uint8_t kRtcDatPin = 8;
constexpr uint8_t kRtcRstPin = 4;

bool isPlausible(const RtcDateTime& value) {
  return value.IsValid() && value.Year() >= 2024 && value.Year() <= 2099;
}
}

TimeService::TimeService()
    : rtcWire(kRtcDatPin, kRtcClkPin, kRtcRstPin), rtc(rtcWire) {
}

bool TimeService::begin() {
  rtc.Begin();
  rtcInitialized = true;

  if (rtc.GetIsWriteProtected()) {
    rtc.SetIsWriteProtected(false);
  }
  if (!rtc.GetIsRunning()) {
    rtc.SetIsRunning(true);
  }

  RtcDateTime now = rtc.GetDateTime();
  available = rtc.IsDateTimeValid() && isPlausible(now);

  if (!available) {
    return false;
  }
  return available;
}

bool TimeService::getNow(CalendarTime& result) {
  result.valid = false;
  if (!available) {
    return false;
  }

  const RtcDateTime now = rtc.GetDateTime();
  if (!rtc.IsDateTimeValid() || !isPlausible(now)) {
    available = false;
    return false;
  }

  result.unixTime = now.Unix32Time();
  result.year = now.Year();
  result.month = now.Month();
  result.day = now.Day();
  result.hour = now.Hour();
  result.minute = now.Minute();
  result.second = now.Second();
  result.dateKey = static_cast<uint32_t>(result.year) * 10000UL +
                   static_cast<uint32_t>(result.month) * 100UL + result.day;
  result.monthKey = static_cast<uint16_t>(result.year * 100U + result.month);
  snprintf(result.timestamp, sizeof(result.timestamp), "%04u-%02u-%02u %02u:%02u:%02u",
           result.year, result.month, result.day, result.hour, result.minute, result.second);
  snprintf(result.date, sizeof(result.date), "%04u-%02u-%02u",
           result.year, result.month, result.day);
  snprintf(result.monthText, sizeof(result.monthText), "%04u-%02u",
           result.year, result.month);

  struct tm utc;
  const time_t epoch = static_cast<time_t>(result.unixTime);
  if (gmtime_r(&epoch, &utc) == nullptr) {
    return false;
  }
  char weekYearText[8];
  strftime(weekYearText, sizeof(weekYearText), "%G", &utc);
  const uint16_t weekYear = static_cast<uint16_t>(atoi(weekYearText));
  char weekText[4];
  strftime(weekText, sizeof(weekText), "%V", &utc);
  result.isoWeek = static_cast<uint8_t>(atoi(weekText));
  result.weekKey = static_cast<uint32_t>(weekYear) * 100UL + result.isoWeek;
  result.valid = true;
  return true;
}

bool TimeService::setUnixTime(uint32_t unixTime) {
  if (!rtcInitialized || unixTime < 1704067200UL) {
    return false;
  }
  RtcDateTime dateTime;
  dateTime.InitWithUnix32Time(unixTime);
  rtc.SetDateTime(dateTime);
  const RtcDateTime written = rtc.GetDateTime();
  available = rtc.IsDateTimeValid() && isPlausible(written);
  return available;
}

bool TimeService::isAvailable() const {
  return available;
}
