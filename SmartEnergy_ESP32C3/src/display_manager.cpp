#include "display_manager.h"

DisplayManager::DisplayManager()
    : currentPage(GraphPage::DASHBOARD),
      metrics{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, ThermalState::NORMAL},
      isInitialized(false) {
}

void DisplayManager::begin() {
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  isInitialized = true;
}

TFT_eSPI& DisplayManager::getTft() {
  return tft;
}

void DisplayManager::update(const DisplayMetrics& newMetrics) {
  metrics = newMetrics;
  if (currentPage == GraphPage::DASHBOARD) renderDashboard();
}

void DisplayManager::setGraphPage(GraphPage page) {
  if (currentPage != page) {
    currentPage = page;
    if (currentPage == GraphPage::DASHBOARD) renderDashboard();
  }
}

GraphPage DisplayManager::getGraphPage() const {
  return currentPage;
}

void DisplayManager::renderDashboard() {
  if (!isInitialized) {
    return;
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.fillRect(0, 6, 128, 20, TFT_BLACK);
  tft.setCursor(2, 6);
  tft.printf("V %.1f V", metrics.voltage);
  tft.fillRect(0, 30, 128, 20, TFT_BLACK);
  tft.setCursor(2, 30);
  tft.printf("I %.2f A", metrics.current);
  tft.fillRect(0, 54, 128, 20, TFT_BLACK);
  tft.setCursor(2, 54);
  tft.printf("P %.0f W", metrics.power);
  tft.fillRect(0, 78, 128, 20, TFT_BLACK);
  tft.setCursor(2, 78);
  tft.printf("E %.2f kWh", metrics.energy);
  tft.fillRect(0, 102, 128, 20, TFT_BLACK);
  tft.setCursor(2, 102);
  tft.printf("T %.1f C", metrics.temperature);
  tft.fillRect(0, 132, 128, 16, TFT_BLACK);
  tft.setCursor(2, 132);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  if (metrics.state == ThermalState::WARM) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  } else if (metrics.state == ThermalState::HOT || metrics.state == ThermalState::SENSOR_FAULT) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }
  tft.printf("STATUS: %s", metrics.state == ThermalState::NORMAL ? "GREEN" :
                           metrics.state == ThermalState::WARM ? "YELLOW" :
                           metrics.state == ThermalState::SENSOR_FAULT ? "FAULT" : "RED");
}

void DisplayManager::renderGraphPage() {
  if (!isInitialized) {
    return;
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.printf("Graph Page %d", static_cast<int>(currentPage));
}
