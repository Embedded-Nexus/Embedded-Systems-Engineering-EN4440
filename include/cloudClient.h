#ifndef CLOUDCLIENT_H
#define CLOUDCLIENT_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

class CloudClient {
public:
    CloudClient();

    // Fetch raw JSON string from an endpoint
    String fetch(const char* url);

    // Extract a key’s value from a JSON string
    String getValue(const String& json, const String& key);

    // Send acknowledgment or JSON data to cloud
    bool postJSON(const char* url, const String& jsonPayload);

private:
    String _lastError;
};

#endif
