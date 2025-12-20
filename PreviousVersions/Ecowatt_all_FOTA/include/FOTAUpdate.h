#ifndef FOTA_UPDATE_H
#define FOTA_UPDATE_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

class FOTAUpdate {
  public:
    FOTAUpdate(const String& firmwareURL, const String& currentVersion = "");
    void checkForUpdate();
    void setVersion(const String& version);
    String getVersion();

  private:
    String _firmwareURL;
    String _currentVersion;
};

#endif
