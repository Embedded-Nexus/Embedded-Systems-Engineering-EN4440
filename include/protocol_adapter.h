#ifndef PROTOCOL_ADAPTER_H
#define PROTOCOL_ADAPTER_H

#include <Arduino.h>
#include <vector>
#include "request_sim.h" 

using namespace std;

namespace ProtocolAdapter {

    // Build Modbus Request Frame
    vector<uint8_t> BuildRequestFrame(uint8_t slaveAddr, uint8_t funcCode, uint16_t startAddr, uint16_t numReg, uint16_t data = 0);

    // Decode struct → Build & Queue frames
    const vector<vector<uint8_t>>& decodeRequestStruct(const RequestSIM &input);

    // Queue accessors
    const vector<vector<uint8_t>>& getFrameQueue();
    void clearFrameQueue();

}  // namespace ProtocolAdapter

#endif
