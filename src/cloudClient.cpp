#include "CloudClient.h"
#include <ESP8266WiFi.h>

CloudClient::CloudClient() {}

String CloudClient::fetch(const char* url) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[CloudClient] WiFi not connected");
        _lastError = "WiFi not connected";
        return "";
    }

    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
      
    // Uncomment the line below for HTTPS endpoints without valid certificates
    // http.setInsecure();

    Serial.printf("[CloudClient] GET %s\n", url);
    int httpCode = http.GET();

    if (httpCode <= 0) {
        _lastError = http.errorToString(httpCode);
        Serial.printf("[CloudClient] HTTP request failed: %s\n", _lastError.c_str());
        http.end();
        return "";
    }

    if (httpCode != HTTP_CODE_OK) {
        _lastError = "Unexpected HTTP code: " + String(httpCode);
        Serial.printf("[CloudClient] %s\n", _lastError.c_str());
        http.end();
        return "";
    }

    String payload = http.getString();
    http.end();

    Serial.println("[CloudClient] HTTP OK, data received");
    return payload;
}

String CloudClient::getValue(const String& json, const String& key) {
    // Locate the key in the JSON string
    int keyIndex = json.indexOf("\"" + key + "\"");
    if (keyIndex == -1) {
        Serial.printf("[CloudClient] Key '%s' not found\n", key.c_str());
        return "";
    }

    // Find the colon after the key
    int colonIndex = json.indexOf(":", keyIndex);
    if (colonIndex == -1) return "";

    // Move to the start of the value
    int valueStart = colonIndex + 1;

    // Skip whitespace and quotes
    while (valueStart < json.length() &&
          (json[valueStart] == ' ' || json[valueStart] == '\"')) {
        valueStart++;
    }

    // Find the end of the value (comma, closing brace, or quote)
    int valueEnd = valueStart;
    while (valueEnd < json.length() &&
          json[valueEnd] != ',' &&
          json[valueEnd] != '}' &&
          json[valueEnd] != '\"') {
        valueEnd++;
    }

    String value = json.substring(valueStart, valueEnd);
    value.trim();
    return value;
}

bool CloudClient::postJSON(const char* url, const String& jsonPayload) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[CloudClient] WiFi not connected (POST aborted)");
        _lastError = "WiFi not connected";
        return false;
    }

    HTTPClient http;
    WiFiClient client;

    if (!http.begin(client, url)) {
        Serial.println("[CloudClient] Failed to begin HTTP POST");
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    Serial.printf("[CloudClient] POST %s\n", url);
    Serial.printf("[CloudClient] Payload: %s\n", jsonPayload.c_str());

    int httpCode = http.POST(jsonPayload);

    if (httpCode <= 0) {
        _lastError = http.errorToString(httpCode);
        Serial.printf("[CloudClient] HTTP POST failed: %s\n", _lastError.c_str());
        http.end();
        return false;
    }

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED) {
        Serial.printf("[CloudClient] ✅ POST successful, code: %d\n", httpCode);
        String response = http.getString();
        Serial.printf("[CloudClient] Server response: %s\n", response.c_str());
        http.end();
        return true;
    } else {
        Serial.printf("[CloudClient] ⚠️ Unexpected POST response: %d\n", httpCode);
    }

    http.end();
    return false;
}

