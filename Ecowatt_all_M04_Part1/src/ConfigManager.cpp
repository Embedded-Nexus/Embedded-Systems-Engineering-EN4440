#include "ConfigManager.h"

ConfigManager& ConfigManager::instance() {
  static ConfigManager cm;
  return cm;
}

void ConfigManager::load() {
  if (!LittleFS.begin()) LittleFS.begin();
  File f = LittleFS.open("/config.json", "r");
  if (!f) {
    cur = {1, 5000, {"voltage", "current", "frequency"}};
    return;
  }
  
  DynamicJsonDocument d(512);
  deserializeJson(d, f);
  cur.configId = d["configId"] | 1;
  cur.acqPeriodMs = d["acqPeriodMs"] | 5000;
  cur.registers.clear();
  if (d["registers"].is<JsonArray>())
    for (auto v : d["registers"].as<JsonArray>())
      cur.registers.push_back(v.as<String>());
  f.close();
}

void ConfigManager::save() {
  DynamicJsonDocument d(512);
  d["configId"] = cur.configId;
  d["acqPeriodMs"] = cur.acqPeriodMs;
  JsonArray arr = d.createNestedArray("registers");
  for (auto& r : cur.registers) arr.add(r);
  File f = LittleFS.open("/config.json", "w");
  serializeJson(d, f);
  f.close();
}

bool ConfigManager::updateFromCloud(const DeviceConfig& cfg, String& result) {
  if (cfg.configId <= cur.configId) { result = "duplicate"; return false; }
  if (cfg.acqPeriodMs < 500 || cfg.acqPeriodMs > 60000) { result = "invalid period"; return false; }

  cur = cfg;
  save();
  result = "ok";
  return true;
}
