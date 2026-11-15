#include "graph_manager.h"

GraphManager::GraphManager()
    : tft(nullptr), currentPage(GraphPage::POWER), head(0), count(0), cursor(0),
      lastY(0), pageNeedsRedraw(true), samplePending(false) {
  for (uint16_t i = 0; i < kMaxSamples; ++i) {
    samples[i].value = 0.0f;
    samples[i].current = 0.0f;
    samples[i].temperature = 0.0f;
    samples[i].timestampMs = 0;
  }
}

void GraphManager::begin(DisplayManager& display) {
  tft = &display.getTft();
}

void GraphManager::setPage(GraphPage page) {
  if (page != currentPage) {
    currentPage = page;
    pageNeedsRedraw = true;
  }
}

void GraphManager::addSample(float power, float current, float temperature) {
  if (!tft) {
    return;
  }

  samples[head].value = power;
  samples[head].current = current;
  samples[head].temperature = temperature;
  samples[head].timestampMs = millis();
  head = (head + 1) % kMaxSamples;
  if (count < kMaxSamples) {
    ++count;
  }
  samplePending = true;
}

void GraphManager::render() {
  if (!tft || currentPage == GraphPage::DASHBOARD) {
    return;
  }

  if (pageNeedsRedraw) {
    drawFrame();
    cursor = 0;
    bool hasPrevious = false;
    for (uint16_t i = 0; i < count; ++i) {
      const uint16_t index = (head + kMaxSamples - count + i) % kMaxSamples;
      drawSample(cursor, samples[index], hasPrevious);
      ++cursor;
      hasPrevious = true;
    }
    cursor %= kMaxSamples;
    pageNeedsRedraw = false;
    samplePending = false;
    return;
  }

  if (samplePending && count > 0) {
    const uint16_t newest = (head + kMaxSamples - 1) % kMaxSamples;
    tft->drawFastVLine(cursor, 30, 124, TFT_BLACK);
    const bool connectPrevious = cursor != 0;
    drawSample(cursor, samples[newest], connectPrevious);
    lastY = static_cast<uint16_t>(scaleY(selectedValue(samples[newest])));
    cursor = (cursor + 1) % kMaxSamples;
    samplePending = false;
  }
}

void GraphManager::drawFrame() {
  tft->fillScreen(TFT_BLACK);
  tft->setTextSize(1);
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->setCursor(4, 6);
  if (currentPage == GraphPage::POWER) tft->print("POWER  W");
  if (currentPage == GraphPage::CURRENT) tft->print("CURRENT  A");
  if (currentPage == GraphPage::TEMPERATURE) tft->print("BREAKER TEMP  C");
  tft->drawRect(0, 28, 128, 128, TFT_DARKGREY);
  for (uint8_t row = 1; row < 4; ++row) {
    const int16_t y = 28 + row * 32;
    tft->drawFastHLine(1, y, 126, TFT_DARKGREY);
  }
  lastY = 0;
}

float GraphManager::selectedValue(const GraphSample& sample) const {
  if (currentPage == GraphPage::CURRENT) return sample.current;
  if (currentPage == GraphPage::TEMPERATURE) return sample.temperature;
  return sample.value;
}

int16_t GraphManager::scaleY(float value) const {
  float maximum = 4000.0f;
  if (currentPage == GraphPage::CURRENT) maximum = 30.0f;
  if (currentPage == GraphPage::TEMPERATURE) maximum = 100.0f;
  if (value < 0.0f) value = 0.0f;
  if (value > maximum) value = maximum;
  return static_cast<int16_t>(154 - (value * 124.0f / maximum));
}

void GraphManager::drawSample(uint16_t x, const GraphSample& sample, bool connectPrevious) {
  const int16_t y = scaleY(selectedValue(sample));
  const uint16_t color = currentPage == GraphPage::POWER ? TFT_GREEN :
                         currentPage == GraphPage::CURRENT ? TFT_CYAN : TFT_YELLOW;
  if (connectPrevious && x > 0) {
    tft->drawLine(x - 1, lastY, x, y, color);
  } else {
    tft->drawPixel(x, y, color);
  }
  lastY = static_cast<uint16_t>(y);
}
