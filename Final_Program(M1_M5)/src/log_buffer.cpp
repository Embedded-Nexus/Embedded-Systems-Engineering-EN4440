#include "log_buffer.h"
#include <time.h>

LogEntry LogBuffer::logs[MAX_LOGS];
int LogBuffer::index = 0;

static String currentTimestamp() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[22];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year+1900, t->tm_mon+1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    return String(buf);
}

void LogBuffer::add(const String& msg) {
    LogEntry &e = logs[index % MAX_LOGS];
    e.timestamp = currentTimestamp();
    e.message   = msg;
    index++;
}

std::vector<LogEntry> LogBuffer::getAll() {
    std::vector<LogEntry> out;
    int count = min(index, MAX_LOGS);
    for (int i = 0; i < count; i++) {
        out.push_back(logs[i]);
    }
    return out;
}
