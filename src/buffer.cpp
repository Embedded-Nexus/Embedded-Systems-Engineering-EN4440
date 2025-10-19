#include "buffer.h"
#include "temporary_buffer.h"
#include "debug_utils.h"
#include "inverter_comm.h"
#include "request_config.h"


namespace Buffer {

    static std::vector<TimedSnapshot> mainBuffer;

    // --------------------------------------------------------------------
    // Append filtered snapshot(s) from TemporaryBuffer
    // --------------------------------------------------------------------
    void appendFromTemporary(const RequestSIM& config) {
        const auto& tempData = TemporaryBuffer::getAll();
        if (tempData.empty()) {
            DEBUG_PRINTLN("[Buffer] ⚠️ Temporary buffer is empty, nothing to append.");
            return;
        }

        // Process each snapshot stored in TemporaryBuffer
        for (const auto& snapshot : tempData) {
            TimedSnapshot filtered;
            filtered.timestamp = snapshot.timestamp;
            filtered.values.assign(NUM_REGISTERS, -1.0f);  // initialize all unread

            // Copy only registers flagged for reading
            for (size_t i = 0; i < snapshot.values.size() && i < NUM_REGISTERS; ++i) {
                if (config.read[i]) {
                    filtered.values[i] = snapshot.values[i];
                }
            }

            mainBuffer.push_back(filtered);

            DEBUG_PRINTF("[Buffer] ✅ Added filtered snapshot at %s\n",
                         filtered.timestamp.c_str());
        }

        DEBUG_PRINTF("[Buffer] 📦 Main buffer now has %d snapshot(s)\n",
                     (int)mainBuffer.size());
    }

    // --------------------------------------------------------------------
    // Retrieve all stored snapshots
    // --------------------------------------------------------------------
    const std::vector<TimedSnapshot>& getAll() {
        return mainBuffer;
    }

    // --------------------------------------------------------------------
    // Clear main buffer
    // --------------------------------------------------------------------
    void clear() {
        mainBuffer.clear();
        DEBUG_PRINTLN("[Buffer] 🧹 Main buffer cleared.");
    }

}  // namespace Buffer
