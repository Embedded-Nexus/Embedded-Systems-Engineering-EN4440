#ifndef REQUEST_SIM_H
#define REQUEST_SIM_H

#include <Arduino.h>
#include "register_map.h"   // gives us REGISTER_COUNT

// Alias for clarity
#define NUM_REGISTERS REGISTER_COUNT

// Structure describing which registers to read/write and their data
struct RequestSIM {
    bool read[NUM_REGISTERS];         // true if this register should be read
    bool write[NUM_REGISTERS];        // true if this register should be written
    uint16_t writeData[NUM_REGISTERS]; // values to write if write[i] == true
};

#endif  // REQUEST_SIM_H
