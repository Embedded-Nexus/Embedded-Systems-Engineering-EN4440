#ifndef TEMPORARY_BUFFER_H
#define TEMPORARY_BUFFER_H

#include <Arduino.h>
#include <vector>
#include "timed_snapshot.h"   // our new structure

using namespace std;

namespace TemporaryBuffer {

    // Global buffer storage for snapshots
    extern vector<TimedSnapshot> buffer;

    // Add a new snapshot (replace or append)
    void update(const TimedSnapshot& newSnapshot);

    // Retrieve all stored snapshots
    const vector<TimedSnapshot>& getAll();

    // Clear buffer
    void clear();

}  // namespace TemporaryBuffer

#endif
