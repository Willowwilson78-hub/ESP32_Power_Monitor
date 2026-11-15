#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "thermal_monitor.h"

enum class GraphPage : uint8_t {
  DASHBOARD = 0,
  POWER = 1,
  CURRENT = 2,
  TEMPERATURE = 3
};

struct DisplayMetrics {
  float voltage;
  float current;
  float power;
  float energy;
  float temperature;
  ThermalState state;
};

class DisplayManager {
public:
  DisplayManager();
  void begin();
  TFT_eSPI& getTft();
  void update(const DisplayMetrics& metrics);
  void setGraphPage(GraphPage page);
  GraphPage getGraphPage() const;
  void renderDashboard();
  void renderGraphPage();

private:
  TFT_eSPI tft;
  GraphPage currentPage;
  DisplayMetrics metrics;
  bool isInitialized;
};

#endif
