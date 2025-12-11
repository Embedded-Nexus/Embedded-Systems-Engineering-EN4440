#ifndef TIMED_SNAPSHOT_H
#define TIMED_SNAPSHOT_H

#include <Arduino.h>
#include <vector>

using namespace std;

// A full register snapshot taken at a specific timestamp
struct TimedSnapshot {
    String timestamp;          // Timestamp of when snapshot was taken
    vector<float> values;      // Length = NUM_REGISTERS, -1 for unread registers
};

#endif  // TIMED_SNAPSHOT_H
