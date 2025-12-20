#include "request_config.h"

namespace RequestConfig {

    RequestSIM buildRequestConfig() {
        RequestSIM req = {};

        // WRITE requests
        req.write[8] = true;          // Export power percentage
        req.writeData[8] = 25;        // Set to 25%

        // READ requests
        req.read[2] = true;           // L1 Phase Frequency
        req.read[8] = true;           // Export power %

        return req;
    }

}
