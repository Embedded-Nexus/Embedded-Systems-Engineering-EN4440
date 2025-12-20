#ifndef REQUEST_CONFIG_H
#define REQUEST_CONFIG_H

#include "protocol_adapter.h"

// 🧩 Configuration namespace for all inverter request presets
namespace RequestConfig {

    // Build a default Modbus request configuration
    RequestSIM buildRequestConfig();

    // (Optional) you can add more later, e.g.:
    // RequestSIM buildStartup();
    // RequestSIM buildDiagnostics();

}

#endif  // REQUEST_CONFIG_H
