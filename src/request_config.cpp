#include "request_config.h"

namespace RequestConfig {

    RequestSIM buildRequestConfig() {
        RequestSIM req = {};
        // READ requests
        req.read[2] = true;           // L1 Phase Frequency
        req.read[8] = true;           // Export power %

        return req;
    }

}