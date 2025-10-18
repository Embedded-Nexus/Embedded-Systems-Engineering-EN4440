#ifndef INVERTER_UTILS_H
#define INVERTER_UTILS_H

#include <Arduino.h>
#include <vector>
using namespace std;

namespace InverterUtils {

    // Convert Modbus binary frame → JSON string
    String frameToJson(const vector<uint8_t>& frame);

    // Convert JSON string → binary frame
    vector<uint8_t> jsonToFrame(const String& response);

    // Cloud API calls (ESP8266)
    String readAPI(const String& jsonFrame, const String& apiKey);
    String writeAPI(const String& jsonFrame, const String& apiKey);

}

#endif  // INVERTER_UTILS_H
