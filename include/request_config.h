#ifndef REQUEST_CONFIG_H
#define REQUEST_CONFIG_H

#include "protocol_adapter.h"
#include "request_sim.h"   // 👈 now we actually know what RequestSIM is!

namespace RequestConfig {
    RequestSIM buildRequestConfig();
}

#endif  // REQUEST_CONFIG_H
