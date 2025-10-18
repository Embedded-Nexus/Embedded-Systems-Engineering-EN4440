#include "buffer.h"
#include "debug_utils.h"
#include <time.h>   // for timestamp generation

namespace Buffer {

    vector<TimedRegister> mainBuffer;

    // Helper: format current local time as "YYYY-MM-DD HH:MM:SS"
    String getCurrentTimestamp() {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char buf[25];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
        return String(buf);
    }

    void appendFromTemporary(const RequestSIM& config) {
        const auto& allData = TemporaryBuffer::getAll();
        String currentTime = getCurrentTimestamp();

        for (const auto& reg : allData) {
            // Include only registers flagged for reading
            if (reg.index < NUM_REGISTERS && config.read[reg.index]) {
                mainBuffer.push_back({reg, currentTime});
            }
        }

        // Debug print
        DEBUG_PRINTF("[Buffer] ✅ Appended %d new filtered registers at %s\n",
                     (int)allData.size(), currentTime.c_str());
    }

    const vector<TimedRegister>& getAll() {
        return mainBuffer;
    }

    void clear() {
        mainBuffer.clear();
        DEBUG_PRINTLN("[Buffer] 🧹 Main buffer cleared.");
    }

}  // namespace Buffer
