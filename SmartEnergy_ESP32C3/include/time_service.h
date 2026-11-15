#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>
#include <RtcDS1302.h>
#include <ThreeWire.h>

struct CalendarTime {
  uint32_t unixTime;
  uint32_t dateKey;
  uint32_t weekKey;
  uint16_t monthKey;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t isoWeek;
  bool valid;
  char timestamp[20];
  char date[11];
  char monthText[8];
};

class TimeService {
public:
  TimeService();
  bool begin();
  bool getNow(CalendarTime& result);
  bool setUnixTime(uint32_t unixTime);
  bool isAvailable() const;

private:
  ThreeWire rtcWire;
  RtcDS1302<ThreeWire> rtc;
  bool rtcInitialized = false;
  bool available = false;
};

#endif
