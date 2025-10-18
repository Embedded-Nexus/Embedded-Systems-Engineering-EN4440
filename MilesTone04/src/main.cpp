#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "protocol_adapter.h"
#include "inverter_comm.h"
#include "debug_utils.h"
#include "request_config.h"
#include "polling_manager.h"   // 🆕 new header

const char* ssid = "Ruchira";
const char* password = "1234567890";

void connectToWiFi() {
    DEBUG_PRINTF("Connecting to Wi-Fi: %s\n", ssid);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("\nWi-Fi connected!");
        DEBUG_PRINTF("IP address: %s\n", WiFi.localIP().toString().c_str());
    } else {
        DEBUG_PRINTLN("\n[Error] Wi-Fi connection failed!");
    }
}

void setup() {
    Serial.begin(9600);
    delay(200);

    DEBUG_PRINTLN("=== Debug Mode Active ===");
    connectToWiFi();

    // 🕒 Initialize polling (every 5 seconds)
    PollingManager::begin(20000);
}

void loop() {
    // 🧭 Handle periodic polling
    PollingManager::handle();
}
