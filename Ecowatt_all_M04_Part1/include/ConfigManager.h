#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct DeviceConfig {
  uint32_t configId;
  uint32_t acqPeriodMs;
  std::vector<String> registers;
};

class ConfigManager {
public:
  static ConfigManager& instance();
  void load();
  void save();
  bool updateFromCloud(const DeviceConfig& cfg, String& result);
  const DeviceConfig& current() const { return cur; }

private:
  DeviceConfig cur;
  ConfigManager() = default;
};
