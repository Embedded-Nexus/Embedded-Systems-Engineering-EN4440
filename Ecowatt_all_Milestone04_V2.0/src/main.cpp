#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Acquisition.h"
#include "Buffer.h"
#include "Uploader.h"
#include "ConfigManager.h"

// ---- CONFIG ----
const char* WIFI_SSID = "Ruchira";
const char* WIFI_PASS = "1234567890";
const String INVERTER_API_KEY =
  "NjhhZWIwNDU1ZDdmMzg3MzNiMTQ5YTIyOjY4YWViMDQ1NWQ3ZjM4NzMzYjE0OWExOA==";
const String CLOUD_BASE_URL = "http://172.20.10.2:5000"; // your PC IP running cloud.py

SampleBuffer gBuf(1024);
EcoWattUploader gUp(CLOUD_BASE_URL, INVERTER_API_KEY, 30000, 600, 3);

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nEcoWatt M2+M3 start");

  ConfigManager::instance().load();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(" IP=");
  Serial.println(WiFi.localIP());

  gBuf.setWatermarks(100, 900);
  gBuf.setCallback([](BufferEvent ev, size_t sz) {
    if (ev == BufferEvent::OVERFLOW) Serial.println("[BUF] OVERFLOW");
    if (ev == BufferEvent::HIGH_WATERMARK) Serial.printf("[BUF] HighWM: %u\n", (unsigned)sz);
  });


  gUp.interval = 10000;
  Serial.println("Forced interval = 5000 ms");
}

void loop() {
  // Serial.println("TickStartted");
  // //Acquisition::tick(gBuf, INVERTER_API_KEY, 5000);  // collect samples every 5 seconds
  // Serial.println("Tick Ended");
  // gUp.periodicUpload(gBuf);                         // upload every 5 seconds
  // delay(10);
  // Serial.printf("Initial upload interval = %lu ms\n", gUp.interval);
}