#include "inverterSIM_utils.h"
#include <sstream>
#include <iomanip>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "debug_utils.h"

namespace InverterUtils {

    ////////////////////// Convert Modbus Frame → JSON //////////////////////
    String frameToJson(const vector<uint8_t>& frame) {
        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0');
        for (size_t i = 0; i < frame.size(); i++) {
            oss << std::setw(2) << (int)frame[i];
        }

        String hexFrame = String(oss.str().c_str());
        String jsonFrame = "{\"frame\":\"" + hexFrame + "\"}";

        DEBUG_PRINTLN("[InverterUtils] Frame converted to JSON:");
        DEBUG_PRINTLN(jsonFrame);
        return jsonFrame;
    }

    ////////////////////// Convert JSON → Modbus Frame //////////////////////
    vector<uint8_t> jsonToFrame(const String& response) {
        int startIdx = response.indexOf("\"frame\":\"");
        if (startIdx == -1) {
            DEBUG_PRINTLN("[InverterUtils] Error: 'frame' key not found.");
            return {};
        }
        startIdx += 9;
        int endIdx = response.indexOf("\"", startIdx);
        if (endIdx == -1) {
            DEBUG_PRINTLN("[InverterUtils] Error: Invalid JSON format.");
            return {};
        }

        String hexString = response.substring(startIdx, endIdx);
        vector<uint8_t> frame;

        for (int i = 0; i < hexString.length(); i += 2) {
            String byteStr = hexString.substring(i, i + 2);
            uint8_t byteVal = (uint8_t) strtol(byteStr.c_str(), nullptr, 16);
            frame.push_back(byteVal);
        }

        DEBUG_PRINTLN("[InverterUtils] JSON converted to Modbus frame.");
        return frame;
    }

    ////////////////////// Cloud API: Read //////////////////////
    String readAPI(const String& jsonFrame, const String& apiKey) {
        WiFiClient client;
        HTTPClient http;
        String response;
        String url = "http://20.15.114.131:8080/api/inverter/read";

        DEBUG_PRINTLN("[InverterUtils] Sending READ request...");
        DEBUG_PRINTF("URL: %s\n", url.c_str());

        if (http.begin(client, url)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", apiKey);

            int httpCode = http.POST(jsonFrame);
            DEBUG_PRINTF("[InverterUtils] HTTP Response Code: %d\n", httpCode);

            if (httpCode > 0) {
                response = http.getString();
                DEBUG_PRINTLN("[InverterUtils] Response:");
                DEBUG_PRINTLN(response);
            } else {
                DEBUG_PRINTLN("[InverterUtils] HTTP POST failed!");
            }

            http.end();
        }
        return response;
    }

    ////////////////////// Cloud API: Write //////////////////////
    String writeAPI(const String& jsonFrame, const String& apiKey) {
        WiFiClient client;
        HTTPClient http;
        String response;
        String url = "http://20.15.114.131:8080/api/inverter/write";

        DEBUG_PRINTLN("[InverterUtils] Sending WRITE request...");
        DEBUG_PRINTF("URL: %s\n", url.c_str());

        if (http.begin(client, url)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", apiKey);

            int httpCode = http.POST(jsonFrame);
            DEBUG_PRINTF("[InverterUtils] HTTP Response Code: %d\n", httpCode);

            if (httpCode > 0) {
                response = http.getString();
                DEBUG_PRINTLN("[InverterUtils] Response:");
                DEBUG_PRINTLN(response);
            } else {
                DEBUG_PRINTLN("[InverterUtils] HTTP POST failed!");
            }

            http.end();
        }
        return response;
    }

}  // namespace InverterUtils
