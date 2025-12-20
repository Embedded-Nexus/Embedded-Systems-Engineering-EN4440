#include "FOTAUpdate.h"

FOTAUpdate::FOTAUpdate(const String& firmwareURL, const String& currentVersion)
: _firmwareURL(firmwareURL), _currentVersion(currentVersion) { }

void FOTAUpdate::setVersion(const String& version) {
  _currentVersion = version;
}

String FOTAUpdate::getVersion() {
  return _currentVersion;
}

void FOTAUpdate::checkForUpdate() {
  Serial.println("[FOTA] Checking for firmware updates...");

  WiFiClient client;
  ESPhttpUpdate.rebootOnUpdate(false); // We'll manually restart

  // Optional: add version header if your server supports it
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); // Visual indicator (optional)
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, _firmwareURL);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[FOTA] No update available.");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("[FOTA] Update successful! Rebooting...");
      delay(1000);
      ESP.restart(); // Force reboot after update
      break;
  }
}
