#pragma once
#include <Arduino.h>
#include <vector>

struct LogEntry {
    String timestamp;
    String message;
};

namespace LogBuffer {
    static const int MAX_LOGS = 100;
    extern LogEntry logs[MAX_LOGS];
    extern int index;

    void add(const String& msg);
    std::vector<LogEntry> getAll();
}
