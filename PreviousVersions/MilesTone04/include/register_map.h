#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <Arduino.h>

// Define the RegisterInfo structure
struct RegisterInfo {
    uint8_t index;       // Register index (R0, R1, ...)
    const char *name;    // Register description
    uint16_t scale;      // Scale factor (e.g. 10, 100)
    const char *unit;    // Unit (e.g. V, A, Hz)
    bool writable;       // true = can write
};

// R0 to R9 (extend if you add more)
const RegisterInfo registerMap[] = {
    {0, "Vac1 / L1 Phase voltage", 10, "V", false},
    {1, "Iac1 / L1 Phase current", 10, "A", false},
    {2, "Fac1 / L1 Phase frequency", 100, "Hz", false},
    {3, "Vpv1 / PV1 input voltage", 10, "V", false},
    {4, "Vpv2 / PV2 input voltage", 10, "V", false},
    {5, "Ipv1 / PV1 input current", 10, "A", false},
    {6, "Ipv2 / PV2 input current", 10, "A", false},
    {7, "Inverter internal temperature", 10, "°C", false},
    {8, "Export power percentage", 1, "%", true},
    {9, "Pac L / Inverter output power", 1, "W", false}
};

// Automatically count registers
const int REGISTER_COUNT = sizeof(registerMap) / sizeof(registerMap[0]);

#endif
