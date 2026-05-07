#include "esp_system.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== ESP32 INFO ===");

  Serial.printf("Chip Model: %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
  Serial.printf("Cores: %d\n", ESP.getChipCores());
  Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("Chip ID: %llu\n", ESP.getEfuseMac());
}

void loop() {}


