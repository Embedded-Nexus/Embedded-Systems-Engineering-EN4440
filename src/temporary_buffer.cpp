#include "temporary_buffer.h"
#include "debug_utils.h"

namespace TemporaryBuffer {

    vector<TimedSnapshot> buffer;

    void update(const TimedSnapshot& newSnapshot) {
        // If you only want to keep the latest one, clear first:
        buffer.clear();

        // Then push the new snapshot
        buffer.push_back(newSnapshot);

        DEBUG_PRINTF("[TempBuffer] 📥 Updated with snapshot at %s (size=%d)\n",
                     newSnapshot.timestamp.c_str(), (int)newSnapshot.values.size());
    }

    const vector<TimedSnapshot>& getAll() {
        return buffer;
    }

    void clear() {
        buffer.clear();
        DEBUG_PRINTLN("[TempBuffer] 🧹 Cleared temporary buffer.");
    }

}  // namespace TemporaryBuffer
