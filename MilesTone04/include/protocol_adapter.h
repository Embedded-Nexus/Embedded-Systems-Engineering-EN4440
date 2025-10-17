#ifndef PROTOCOL_ADAPTER_H
#define PROTOCOL_ADAPTER_H

#include <Arduino.h>
#include <vector>
using namespace std;

#define NUM_REGISTERS 15  // R0–R14

// Struct describing what operations to perform
struct RequestSIM {
    bool read[NUM_REGISTERS];       // read[i] → true if register Ri should be read
    bool write[NUM_REGISTERS];      // write[i] → true if register Ri should be written
    uint16_t writeData[NUM_REGISTERS]; // value to write when write[i] == true
};

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
