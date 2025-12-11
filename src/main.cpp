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
#include "power_estimator.h"

const char* ssid     = "dinujaya";
const char* password = "helloworld";

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
void printErrorLogs() {
    auto logs = LogBuffer::getAll();

    Serial.println("\n================ ERROR LOGS ================");
    for (auto &e : logs) {
        Serial.printf("%s | %s\n",
                      e.timestamp.c_str(),
                      e.message.c_str());
    }
    Serial.println("===========================================\n");
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
    UploadManager::begin("http://192.168.137.1:5000/data","http://192.168.137.1:5000/config","http://192.168.137.1:5000/commands");


    DEBUG_PRINTLN("[System] ✅ Setup complete.");
    pe_begin(5000); // report every 5000 ms

}

void loop() {
    unsigned long cycleStart = millis();
    // 🧭 Handle periodic polling cycle
    // inside loop(), as early as possible
    static unsigned long __pe_last = 0;
    unsigned long __pe_now = millis();
    if (__pe_last == 0) __pe_last = __pe_now;
    unsigned long __pe_dt = __pe_now - __pe_last;
    __pe_last = __pe_now;

    // default assume dt is idle; other modules will add cpu/wifi on top
    pe_addIdleMs(__pe_dt);

    PollingManager::handle();
    UploadManager::handle();
    pe_tickAndMaybePrint();

    // ---- Log print every 60 seconds ----
    static unsigned long lastLogTime = 0;
    unsigned long now = millis();

    if (now - lastLogTime >= 60000) {   // 60 seconds
        lastLogTime = now;
        printErrorLogs();
    }

    unsigned long elapsed = millis() - cycleStart;

    // Polling interval taken from cloud config
    unsigned long interval = pollingInterval;      

    if (elapsed < interval) {

    unsigned long sleepMs = interval - elapsed;
    if (sleepMs > 1000) sleepMs -= 200;

    // Enable Wi-Fi modem sleep (safe mode)
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

    // -------- POWER ESTIMATOR FIX --------
    // Count sleep time
    pe_addSleepMs(sleepMs);

    pe_subtractIdleMs(sleepMs);
    delay(sleepMs);
}


    // Disable modem sleep so heavy ops run smooth
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    // --------- Restore clock after wake ---------
    system_update_cpu_freq(SYS_CPU_80MHZ);


}
