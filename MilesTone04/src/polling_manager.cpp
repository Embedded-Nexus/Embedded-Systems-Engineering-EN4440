#include "polling_manager.h"

namespace {
    unsigned long lastPollTime = 0;
    unsigned long pollInterval = 5000;
}

namespace PollingManager {

    void begin(unsigned long intervalMs) {
        pollInterval = intervalMs;
        lastPollTime = millis();
        DEBUG_PRINTF("[PollingManager] Initialized (interval = %lu ms)\n", pollInterval);
    }

    void handle() {
        unsigned long now = millis();

        // Check if it’s time to poll again
        if (now - lastPollTime >= pollInterval) {
            lastPollTime = now;
            DEBUG_PRINTLN("[PollingManager] ⏱ Starting new polling cycle...");

            // Get request config from request_config.h
            RequestSIM req = buildRequestConfig();
            // Decode to frames
            const auto& frames = ProtocolAdapter::decodeRequestStruct(req);
            // Send frames to inverter
            InverterSim::processFrameQueue(frames);

            DEBUG_PRINTLN("[PollingManager] ✅ Polling cycle complete.\n");
        }
    }

}  // namespace PollingManager
