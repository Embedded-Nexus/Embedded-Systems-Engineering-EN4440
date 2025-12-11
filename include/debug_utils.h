#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>
#include "log_buffer.h"   // <-- ADD THIS

// Turn debugging on/off here
#define DEBUG_ENABLED true

// Simple flag to prevent concurrent Serial access (not true mutex, but helps)
extern bool _serial_busy;

#if DEBUG_ENABLED

// ----------- PRINT WITHOUT TIMESTAMP (Serial only) -----------
#define _DEBUG_SERIAL_PRINT(x)    do { while(_serial_busy) yield(); _serial_busy = true; Serial.print(x); _serial_busy = false; } while(0)
#define _DEBUG_SERIAL_PRINTLN(x)  do { while(_serial_busy) yield(); _serial_busy = true; Serial.println(x); _serial_busy = false; } while(0)

// ----------- MAIN LOGGING WRAPPERS (Serial + RAM buffer) -----------

#define DEBUG_PRINT(x)           \
    do {                         \
        String _msg = String(x); \
        _DEBUG_SERIAL_PRINT(_msg); \
        LogBuffer::add(_msg);    \
    } while(0)

#define DEBUG_PRINTLN(x)         \
    do {                         \
        String _msg = String(x); \
        _DEBUG_SERIAL_PRINTLN(_msg); \
        LogBuffer::add(_msg);    \
    } while(0)

#define DEBUG_PRINTF(fmt, ...)                       \
    do {                                             \
        char _buf[200];                              \
        snprintf(_buf, sizeof(_buf), fmt, ##__VA_ARGS__); \
        _DEBUG_SERIAL_PRINT(_buf);                   \
        LogBuffer::add(String(_buf));                \
        if (strlen(_buf) > 100) Serial.flush();      \
    } while(0)

#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

#endif
