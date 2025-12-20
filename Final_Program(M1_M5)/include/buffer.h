#ifndef BUFFER_H
#define BUFFER_H

#include <Arduino.h>
#include <vector>
#include "timed_snapshot.h"
#include "inverter_comm.h"   // for NUM_REGISTERS, RequestSIM
#include "request_config.h"

namespace Buffer {

    void appendFromTemporary(const RequestSIM& config);
    const std::vector<TimedSnapshot>& getAll();
    void clear();
    bool hasOverflowed();

}  // namespace Buffer

#endif
