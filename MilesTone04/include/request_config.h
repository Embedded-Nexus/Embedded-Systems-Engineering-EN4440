#ifndef REQUEST_CONFIG_H
#define REQUEST_CONFIG_H

#include "protocol_adapter.h"

// 🧱 Predefined inverter request configuration
inline RequestSIM buildRequestConfig() {
    RequestSIM req = {};

    // WRITE request
    req.write[8] = true;          // Export power percentage
    req.writeData[8] = 25;        // Set to 25%

    // READ requests
    req.read[2] = true;           // L1 Phase Frequency
    req.read[8] = true;           // Export power %

    return req;
}

#endif  // REQUEST_CONFIG_H
