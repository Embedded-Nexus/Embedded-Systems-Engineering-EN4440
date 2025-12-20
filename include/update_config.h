#ifndef UPDATE_CONFIG_H
#define UPDATE_CONFIG_H

#include <Arduino.h>
#include "request_sim.h"

namespace UpdateConfig {
    void updateFromCloud(const String& json);
    unsigned long getLastInterval();
    String getLastVersion();
}

#endif  // UPDATE_CONFIG_H
