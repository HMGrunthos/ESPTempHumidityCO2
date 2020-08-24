/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESPAsyncWebServer.h>
#include <DHTesp.h>

#include "MHZ.h"
#include "ManageWiFi.h"
#include "indexHTML.h"

// Pin for uart reading
#define MH_Z14_RX D7
#define MH_Z14_TX D6

// MHZ Interface object
MHZ co2(MH_Z14_RX, MH_Z14_TX, MHZ14A);

static volatile bool mhzDoCalibrate = false;

// Digital pin connected to the DHT sensor
#define DHTPIN 17
// Uncomment the type of sensor in use:
#define DHTTYPE DHT22

// DHT Interface object
DHTesp dht;

// Current temperature, humidity and CO2 readings
struct {
  struct {
    struct {
      float reading;
      unsigned long int lastUpdate;
    } temp, rh;
  } dht;
  struct {
    float temp;
    float co2;
    unsigned long int lastUpdate;
  } mhz;
} currentReadings;

// Create NTP objects
WiFiUDP ntpUDP;
NTPClient timeClient = NTPClient(ntpUDP);

// AsyncWebServer object
static AsyncWebServer *server = NULL;

void setup()
{
  memset(&currentReadings, 0, sizeof(currentReadings));
  currentReadings.dht.temp.reading = NAN;
  currentReadings.dht.rh.reading = NAN;
  currentReadings.mhz.temp = NAN;
  currentReadings.mhz.co2 = NAN;

  // Serial port for debugging purposes
  Serial.begin(115200);
  do {
    delay(200);
  } while(!Serial);

  // Configure the sensor interfaces
  dht.setup(DHTPIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 17
  co2.setDebug(true);

  // Try and connect to WiFi
  TryConnect("EnvMonConfigAP");

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Start the network services
  timeClient.begin();
  startServer();
}

void loop()
{
  // Generally, you should use "unsigned long" for variables that hold time
  // The value will quickly become too large for an int to store
  static unsigned long previousMillis = 0;    // will store last time DHT was updated

  // Updates readings every 10 seconds
  const int long INTERVAL = 10000;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL) {
    // Save the last time you updated the DHT values
    previousMillis = currentMillis;

    // Read temperature as Celsius (the default)
    float newDHTTemp = dht.getTemperature();
    // if temperature read failed, don't change t value
    if (isnan(newDHTTemp)) {
      Serial.println("Failed to read from DHT sensor (Temperature).");
    }
    else {
      currentReadings.dht.temp.reading = newDHTTemp;
      currentReadings.dht.temp.lastUpdate = currentMillis;
      Serial.println(currentReadings.dht.temp.reading);
    }

    // Read Humidity
    float newDHTRH = dht.getHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newDHTRH)) {
      Serial.println("Failed to read from DHT sensor (Humidity).");
    } else {
      currentReadings.dht.rh.reading = newDHTRH;
      currentReadings.dht.rh.lastUpdate = currentMillis;
      Serial.println(currentReadings.dht.rh.reading);
    }

    // Update or calibrate co2 sensor
    if(co2.isReady()) {
      if(mhzDoCalibrate == true) {
        mhzDoCalibrate = false;
        if(co2.calibrate()) {
          Serial.println("Failed to calibrate MH-Z14A sensor!");
        }
      } else {
        // Read CO2
        int newMHZCO2 = co2.readCO2UART();
        // if CO2 read failed, don't change h value 
        if (newMHZCO2 > 0) {
          currentReadings.mhz.co2 = (float)newMHZCO2;
          currentReadings.mhz.temp = (float)co2.getLastTemperature();
          currentReadings.mhz.lastUpdate = currentMillis;
          Serial.println(currentReadings.mhz.co2);
          Serial.println(currentReadings.mhz.temp);
        } else {
          Serial.println("Failed to read from MH-Z14A sensor!");
        }
      }
    }

    // Keep an eye on the WiFi connection and reconnect or reboot as necessary
    if(CheckWiFi()) {
      // If the WiFi is up then do an NTP update
      timeClient.update();
    }

    if(timeClient.getEpochTime() > 1500000000) {
      Serial.println(timeClient.getFormattedTime());
    }
  }
}

void startServer()
{
  // Serial.println("Starting web server.");
  if(!server) {
    server = new AsyncWebServer(80);

    // Route for root / web page
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });
    server->on("/tempDHT", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", requestHandler("TEMPDHT").c_str());
    });
    server->on("/tempMHZ", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", requestHandler("TEMPMHZ").c_str());
    });
    server->on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", requestHandler("HUMIDITY").c_str());
    });
    server->on("/co2", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", requestHandler("CO2").c_str());
    });
    server->on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", requestHandler("UPTIME").c_str());
    });
    server->on("/calibrateCO2", HTTP_GET, [](AsyncWebServerRequest *request){
      mhzDoCalibrate = true;
      request->send_P(200, "text/plain", "OK");
    });

    server->begin();
    // Serial.println("Web server started.");
  }
}

void stopServer()
{
  // Serial.println("Stopping web server.");
  if(server) {
    server->end();
    delay(100);
    delete server;
    server = NULL;
    // Serial.println("Web server stopped.");
  }
}

// Generate the responses to HTTP requests
String requestHandler(const String &var)
{
  String measurement;
  unsigned long int lastUpdate;

  if(var == "TEMPDHT") {
    measurement = String(currentReadings.dht.temp.reading, 1);
    lastUpdate = currentReadings.dht.temp.lastUpdate;
  } else if(var == "TEMPMHZ") {
    measurement = String(currentReadings.mhz.temp, 0);
    lastUpdate = currentReadings.mhz.lastUpdate;
  } else if(var == "HUMIDITY") {
    measurement = String(currentReadings.dht.rh.reading, 0);
    lastUpdate = currentReadings.dht.rh.lastUpdate;
  } else if(var == "CO2") {
    measurement = String(currentReadings.mhz.co2, 0);
    lastUpdate = currentReadings.mhz.lastUpdate;
  } else if(var == "UPTIME") {
    measurement = String(millis());
    lastUpdate = 1;    
  } else {
    return String();
  }

  unsigned long int age = lastUpdate > 0 ? millis() - lastUpdate : 0; // Return 0 if the field has never been updated
  return measurement + "," + String(age);
}
