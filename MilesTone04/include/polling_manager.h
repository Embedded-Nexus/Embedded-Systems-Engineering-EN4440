#ifndef POLLING_MANAGER_H
#define POLLING_MANAGER_H

#include <Arduino.h>
#include "protocol_adapter.h"
#include "inverter_comm.h"
#include "request_config.h"
#include "debug_utils.h"

namespace PollingManager {

    // Declare functions normally — not inline
    void begin(unsigned long intervalMs = 5000);
    void handle();

}  // namespace PollingManager

#endif  // POLLING_MANAGER_H
