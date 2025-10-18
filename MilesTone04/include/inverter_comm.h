#ifndef INVERTER_COMM_H
#define INVERTER_COMM_H

#include <Arduino.h>
#include <vector>
using namespace std;

namespace InverterSim {

    // Converts frame to JSON for Cloud API
    String frameToJson(const vector<uint8_t>& frame);

    // API functions for inverter simulator
    String readAPI(const String& jsonFrame, const String& apiKey);
    String writeAPI(const String& jsonFrame, const String& apiKey);

    // Main function to send a frame to the inverter simulator
    bool sendFrameToInverter(const vector<uint8_t>& frame);

    // Process all frames in queue with retry logic
    void processFrameQueue(const vector<vector<uint8_t>>& frames);

    void processResponseFrame(const String& response, uint16_t startAddr);

    bool validateCRC(const vector<uint8_t>& frame);

    void decodeResponseFrame(const vector<uint8_t>& frame, uint16_t startAddr);


}

#endif
