#ifndef DECODED_REGISTERS_H
#define DECODED_REGISTERS_H

#include <Arduino.h>

struct DecodedRegisters {
    uint8_t index;        // Register index (R0, R1, ...)
    String name;          // Human-readable name
    float scaledValue;    // Scaled (human) value
    uint16_t rawValue;    // Raw Modbus 16-bit value
    String unit;          // Unit string
};

#endif
