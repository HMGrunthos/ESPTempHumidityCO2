// The principal purpose of this file is to ensure that the HTTP server
// used in access point negotiation doesn't clash with the one used to
// present the device's own web page.

#include "WiFiConnect.h"

WiFiConnect wc;
  
void TryConnect(const String &deviceName)
{
  // Connect to Wi-Fi
  wc.setAPName(deviceName.c_str());
  wc.setAPStaticIPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  wc.setAPModeTimeoutMins(3);
  wc.setDebug(true);
  wc.setRetryAttempts(4);
  wc.setConnectionTimeoutSecs(20);
  // wc.resetSettings(); // Helper to remove the stored wifi connection, comment out after first upload and re upload

  // Serial.println("Attempting connection.");
  if(!wc.autoConnect()) { // Try to connect to wifi
    // Serial.println("Failed to connect. Entering portal.");
    wc.startConfigurationPortal(AP_RESET); // If not connected show the configuration portal
    // Serial.println("Exiting portal.");
    delay(100);
    ESP.restart(); // If we successfully connect then reboot and reconnect to avoid problems with other servers on port 80
  }
  wc.setRetryAttempts(1);
  wc.setConnectionTimeoutSecs(10);
}

bool CheckWiFi()
{
  const int NRECONNECTTRIES = 10;
  static int reconnectTries = NRECONNECTTRIES;
  const int RETRYPERIOD = 10;
  static int everyN = RETRYPERIOD;
  static enum {
    Connected,
    Lost
  } connectionState = Connected;

  // Serial.println("Connection Test.");
  // Wifi Dies? Start Portal Again
  if((connectionState == Lost) || (WiFi.status() != WL_CONNECTED)) {
    connectionState = Lost;
    // Serial.println("WiFi connection lost.");

    if(--everyN == 0) {
      everyN = RETRYPERIOD;
      if(!wc.autoConnect()) {
        // Serial.print("Could not reconnect. Tries :");
        // Serial.print(reconnectTries);
        // Serial.println(":");
        if(--reconnectTries == 0) {
          // Could not reconnect and now out of tries so reboot to force us into the AP mode
          delay(100);
          ESP.restart(); // If we successfully connect then reboot and reconnect to avoid problems with other servers on port 80
        }
      } else {
        connectionState = Connected;
        // Serial.println("Reconnected.");
        reconnectTries = NRECONNECTTRIES;
      }
    }
  } else {
    connectionState = Connected;
  }

  return connectionState == Connected;
}
