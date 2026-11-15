#ifndef GRAPH_MANAGER_H
#define GRAPH_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "display_manager.h"

struct GraphSample {
  float value;
  float current;
  float temperature;
  uint32_t timestampMs;
};

class GraphManager {
public:
  static constexpr uint16_t kMaxSamples = 128;

  GraphManager();
  void begin(DisplayManager& display);
  void setPage(GraphPage page);
  void addSample(float power, float current, float temperature);
  void render();

private:
  void drawFrame();
  void drawSample(uint16_t x, const GraphSample& sample, bool connectPrevious);
  int16_t scaleY(float value) const;
  float selectedValue(const GraphSample& sample) const;

  TFT_eSPI* tft;
  GraphPage currentPage;
  GraphSample samples[kMaxSamples];
  uint16_t head;
  uint16_t count;
  uint16_t cursor;
  uint16_t lastY;
  bool pageNeedsRedraw;
  bool samplePending;
};

#endif
