#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>                      // 🕒 For NTP
#include "protocol_adapter.h"
#include "inverter_comm.h"
#include "debug_utils.h"
#include "request_config.h"
#include "polling_manager.h"
#include "upload_manager.h"
#include "request_sim.h"

const char* ssid     = "Ruchira";
const char* password = "1234567890";

// 🌐 Wi-Fi + NTP Setup
void connectToWiFiAndSyncTime() {
    DEBUG_PRINTF("Connecting to Wi-Fi: %s\n", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("\n✅ Wi-Fi connected!");
        DEBUG_PRINTF("IP address: %s\n", WiFi.localIP().toString().c_str());

        // 🌍 Configure NTP — change 5.5 * 3600 to your UTC offset if needed
        configTime(5.5 * 3600, 0, "pool.ntp.org", "time.nist.gov");

        DEBUG_PRINTLN("⏳ Syncing time with NTP servers...");
        time_t now = time(nullptr);
        int retries = 0;
        while (now < 1000000000 && retries < 20) {  // wait until valid epoch time
            delay(500);
            DEBUG_PRINT(".");
            now = time(nullptr);
            retries++;
        }

        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        DEBUG_PRINTF(
            "\n🕒 Time synchronized: %04d-%02d-%02d %02d:%02d:%02d\n",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec
        );
    } else {
        DEBUG_PRINTLN("\n❌ [Error] Wi-Fi connection failed!");
    }
}

void setup() {
    Serial.begin(9600);
    delay(200);

    DEBUG_PRINTLN("=== Debug Mode Active ===");

    //Create the Configuration(default)
    requestSim = RequestConfig::buildRequestConfig();
    // 🌐 Connect to Wi-Fi and get real-world time
    connectToWiFiAndSyncTime();

    // 🕒 Initialize polling (every 10 seconds)
    PollingManager::begin(pollingInterval);
    UploadManager::begin("http://172.20.10.4:5000/data","http://172.20.10.4:5000/config","http://172.20.10.4:5000/commands");


    DEBUG_PRINTLN("[System] ✅ Setup complete.");
}

void loop() {
    // 🧭 Handle periodic polling cycle
    PollingManager::handle();
    UploadManager::handle();
}
