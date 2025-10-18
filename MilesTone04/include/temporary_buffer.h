#ifndef TEMPORARY_BUFFER_H
#define TEMPORARY_BUFFER_H

#include <Arduino.h>
#include <vector>
#include "inverter_comm.h"  // for DecodedRegisters struct

using namespace std;

namespace TemporaryBuffer {

    // Holds the latest decoded register values
    extern vector<DecodedRegisters> buffer;

    // Update or insert new register data (replaces existing entries)
    void update(const vector<DecodedRegisters>& newData);

    // Retrieve all current decoded values
    const vector<DecodedRegisters>& getAll();

    // Clear buffer contents
    void clear();

}  // namespace TemporaryBuffer

#endif
