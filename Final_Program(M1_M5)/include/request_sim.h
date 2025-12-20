#ifndef REQUEST_SIM_H
#define REQUEST_SIM_H

#include <Arduino.h>
#include "register_map.h"   // defines REGISTER_COUNT

#define NUM_REGISTERS REGISTER_COUNT

struct RequestSIM {
    bool read[NUM_REGISTERS];
    bool write[NUM_REGISTERS];
    uint16_t writeData[NUM_REGISTERS];

    void clear();
};

// 👇 Declare the global instance (defined in request_sim.cpp)
extern RequestSIM requestSim;
extern unsigned long pollingInterval;  // ms between polling cycles


void printGlobalRequestSim();  // helper to print current global config


#endif  // REQUEST_SIM_H
