#include "update_config.h"

static unsigned long lastInterval = 0;
static String lastVersion = "unknown";

// ✅ Use the global instance
extern RequestSIM requestSim;

namespace UpdateConfig {

    void updateFromCloud(const String& json) {
        requestSim.clear();

        // --- Parse reg_read array ---
        int start = json.indexOf("\"reg_read\"");
        if (start == -1) {
            Serial.println("[UpdateConfig] ⚠️ reg_read not found in JSON");
        } else {
            int arrayStart = json.indexOf('[', start);
            int arrayEnd   = json.indexOf(']', arrayStart);
            if (arrayStart != -1 && arrayEnd != -1) {
                String arrayStr = json.substring(arrayStart + 1, arrayEnd);
                arrayStr.trim();

                int idx = 0;
                int lastPos = 0;
                while (idx < NUM_REGISTERS && lastPos < arrayStr.length()) {
                    int comma = arrayStr.indexOf(',', lastPos);
                    String token = (comma == -1)
                        ? arrayStr.substring(lastPos)
                        : arrayStr.substring(lastPos, comma);

                    token.trim();
                    requestSim.read[idx] = (token.toInt() != 0);

                    idx++;
                    if (comma == -1) break;
                    lastPos = comma + 1;
                }

                Serial.println("[UpdateConfig] ✅ reg_read[] updated from config");
            }
        }

        // --- Parse interval ---
        int intervalKey = json.indexOf("\"interval\"");
        if (intervalKey != -1) {
            int colon = json.indexOf(':', intervalKey);
            if (colon != -1) {
                int end = json.indexOf(',', colon);
                if (end == -1) end = json.indexOf('}', colon);
                String val = json.substring(colon + 1, end);
                val.trim();
                unsigned long parsed = val.toInt();

                if (parsed > 0) {
                    pollingInterval = parsed;
                    Serial.printf("[UpdateConfig] ⏱️ Interval updated globally: %lu ms\n", pollingInterval);
                } else {
                    Serial.println("[UpdateConfig] ⚠️ Invalid interval value");
                }
            }
        }


        // --- Parse version ---
        int versionKey = json.indexOf("\"version\"");
        if (versionKey != -1) {
            int quote1 = json.indexOf('"', versionKey + 9);
            int quote2 = json.indexOf('"', quote1 + 1);
            if (quote1 != -1 && quote2 != -1) {
                lastVersion = json.substring(quote1 + 1, quote2);
                Serial.printf("[UpdateConfig] 🧩 Version: %s\n", lastVersion.c_str());
            }
        }

        // Debug summary
        Serial.println("[UpdateConfig] ✅ Final requestSim.read[]:");
        for (int i = 0; i < NUM_REGISTERS; i++) {
            Serial.printf("  read[%d] = %d\n", i, requestSim.read[i]);
        }
    }

    unsigned long getLastInterval() { return lastInterval; }
    String getLastVersion() { return lastVersion; }

}  // namespace UpdateConfig
