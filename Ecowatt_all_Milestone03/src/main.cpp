#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Acquisition.h"
#include "Buffer.h"
#include "Uploader.h"

// ---- CONFIG ----
const char* WIFI_SSID = "Ruchira";
const char* WIFI_PASS = "1234567890";
const String INVERTER_API_KEY =
  "NjhhZWIwNDU1ZDdmMzg3MzNiMTQ5YTIyOjY4YWViMDQ1NWQ3ZjM4NzMzYjE0OWExOA==";
const String CLOUD_BASE_URL = "http://172.20.10.2:5000"; // set to your PC IP running cloud.py

SampleBuffer gBuf(1024);
EcoWattUploader gUp(CLOUD_BASE_URL, INVERTER_API_KEY, 10000, 600, 3);

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nEcoWatt M2+M3 start");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  while (WiFi.status()!=WL_CONNECTED){ delay(500); Serial.print("."); }
  Serial.print(" IP="); Serial.println(WiFi.localIP());

  gBuf.setWatermarks(100, 900);
  gBuf.setCallback([](BufferEvent ev, size_t sz){
    if (ev==BufferEvent::OVERFLOW) Serial.println("[BUF] OVERFLOW");
    if (ev==BufferEvent::HIGH_WATERMARK) Serial.printf("[BUF] HighWM: %u\n", (unsigned)sz);
  });
}

void loop() {
  // Milestone 2: poll inverter & append quantized u16 samples
  Acquisition::tick(gBuf, INVERTER_API_KEY, 5000);

  // Milestone 3: periodic finalize->compress->upload (15s demo)
  gUp.periodicUpload(gBuf);
  delay(10);
}
