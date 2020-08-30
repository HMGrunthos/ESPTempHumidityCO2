/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <ESPAsyncWebServer.h>
#include <SPIFFSLogger.h>
#include <DHTesp.h>

#include "MHZ.h"
#include "ManageWiFi.h"
#include "miniz.h"

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

#define CLEARED 0
#define DHTTEMP (1<<0)
#define DHTRH (1<<1)
#define MHZTEMP (1<<2)
#define MHZCO2 (1<<3)
#define ALLUPDATED (DHTTEMP | DHTRH | MHZTEMP | MHZCO2)

// Current temperature, humidity and CO2 readings for house keeping
struct {
  struct {
    struct {
      float *reading;
      unsigned long int lastUpdate;
    } temp, rh;
  } dht;
  struct {
    int8_t *temp;
    uint16_t *co2;
    unsigned long int lastUpdate;
  } mhz;
  uint_fast8_t updateState;
} currentReadings;

// Logger data
struct EnvData {
  struct {
    float temp;
    float humidity;
  } dht;
  struct {
    int8_t temp;
    uint16_t co2;
  } mhz;
} dataToLog;
SPIFFSLogger<struct EnvData> logger("/log", 70);
static bool loggerInitialised = false;

// AsyncWebServer object
static AsyncWebServer *server = NULL;

void setup()
{
  memset(&dataToLog, 0, sizeof(dataToLog));
  dataToLog.dht.temp = NAN;
  dataToLog.dht.humidity = NAN;
  memset(&currentReadings, 0, sizeof(currentReadings));
  currentReadings.dht.temp.reading = &dataToLog.dht.temp;
  currentReadings.dht.rh.reading = &dataToLog.dht.humidity;
  currentReadings.mhz.temp = &dataToLog.mhz.temp;
  currentReadings.mhz.co2 = &dataToLog.mhz.co2;

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

  // Configure time service
  configTime(0, 0, "pool.ntp.org");
  InitTimeZone();

  // initialize SPIFFS
  SPIFFS.begin();

  // Start the services
  startServer();
}

void loop()
{
  // Generally, you should use "unsigned long" for variables that hold time
  // The value will quickly become too large for an int to store
  static unsigned long previousMillis = 0;    // will store last time DHT was updated

  // Updates readings every 10 seconds
  const int long INTERVAL = 10000;

  static uint8_t mostAvailMeas = 0;
        
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
      *currentReadings.dht.temp.reading = newDHTTemp;
      currentReadings.dht.temp.lastUpdate = currentMillis;
      currentReadings.updateState |= DHTTEMP;
      Serial.println(*currentReadings.dht.temp.reading);
    }

    // Read Humidity
    float newDHTRH = dht.getHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newDHTRH)) {
      Serial.println("Failed to read from DHT sensor (Humidity).");
    } else {
      *currentReadings.dht.rh.reading = newDHTRH;
      currentReadings.dht.rh.lastUpdate = currentMillis;
      currentReadings.updateState |= DHTRH;
      Serial.println(*currentReadings.dht.rh.reading);
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
          *currentReadings.mhz.co2 = (uint16_t)newMHZCO2;
          *currentReadings.mhz.temp = (int8_t)co2.getLastTemperature();
          currentReadings.mhz.lastUpdate = currentMillis;
          currentReadings.updateState |= MHZTEMP;
          currentReadings.updateState |= MHZCO2;
          Serial.println(*currentReadings.mhz.co2);
          Serial.println(*currentReadings.mhz.temp);
        } else {
          Serial.println("Failed to read from MH-Z14A sensor!");
        }
      }
    }

    mostAvailMeas |= currentReadings.updateState;
    if(mostAvailMeas != 0 && (currentReadings.updateState == mostAvailMeas)) {
      if(loggerInitialised == true) {
        logger.write(dataToLog);
        time_t tNow;
        struct tm *utcNow;
        time(&tNow);
        utcNow = gmtime (&tNow);
        Serial.print("Logged values on: ");
        Serial.print(asctime(utcNow));
      } else {
        Serial.println("Didn't log (uninitialised).");
      }
      currentReadings.updateState = CLEARED;
    }

    // Keep an eye on the WiFi connection and reconnect or reboot as necessary
    if(CheckWiFi()) {
      // In here if the wifi is up
    }
  }

  // Initialise and update the logger if the time service has been initialised
  if(loggerInitialised == false) {
    if(time(NULL) > 1500000000) {
      logger.init();
      loggerInitialised = true;
      Serial.println("Logger initialised.");
    }
  } else {
    // Update and rotate logs
    logger.process();
  }

  yield();
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
      request->send(200, "text/plain", requestHandler("TEMPDHT").c_str());
    });
    server->on("/tempMHZ", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", requestHandler("TEMPMHZ").c_str());
    });
    server->on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", requestHandler("HUMIDITY").c_str());
    });
    server->on("/co2", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", requestHandler("CO2").c_str());
    });
    server->on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", requestHandler("UPTIME").c_str());
    });
    server->on("/calibrateCO2", HTTP_GET, [](AsyncWebServerRequest *request){
      mhzDoCalibrate = true;
      request->send(200, "text/plain", "OK");
    });
    server->on("/csv", HTTP_GET, fetchCSVData);

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
    measurement = String(*currentReadings.dht.temp.reading, 1);
    lastUpdate = currentReadings.dht.temp.lastUpdate;
  } else if(var == "TEMPMHZ") {
    measurement = String(*currentReadings.mhz.temp);
    lastUpdate = currentReadings.mhz.lastUpdate;
  } else if(var == "HUMIDITY") {
    measurement = String(*currentReadings.dht.rh.reading, 0);
    lastUpdate = currentReadings.dht.rh.lastUpdate;
  } else if(var == "CO2") {
    measurement = String(*currentReadings.mhz.co2);
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

void InitTimeZone()
{
  struct timeval tv = {0, 0};
  struct timezone tz = {0, 0};
  settimeofday(&tv, &tz);
  setenv("TZ", "Etc/UTC", 0);
  tzset();
}

void fetchCSVData(AsyncWebServerRequest *request)
{
  time_t tNow = time(NULL);
  uint_fast32_t currentLine = 0;

 Serial.print("Search start...");
  AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [tNow, currentLine](uint8_t *buffer, size_t maxLen, size_t index) mutable -> size_t {
    uint_fast16_t nChars;
    if(currentLine == 0) {
      PGM_P csvHeader = PSTR("unixTime,dhtTemp,dhtRH,mhzTemp,mhzCO2");

      strncpy_P((char*)buffer, csvHeader, maxLen);
      currentLine++;
      nChars = strlen((char*)buffer);
    } else if(loggerInitialised == true) {
      SPIFFSLogData<EnvData> data[16];
      char *cBPtr = (char*)buffer;

      // Run the query
      size_t nLinesRead = logger.readRowsBetween(data,                      // output
                                                 tNow - (60 * 60 * 36),     // time start (inclusive)
                                                 tNow,                      // time end (inclusive)
                                                 currentLine - 1,           // start index within results
                                                 16);                       // max number of rows to fetch

      nChars = 0;
      for(size_t cLine = 0; cLine < nLinesRead; cLine++) {
        char tBuff[32];
        int rVal = snprintf_P(tBuff, sizeof(tBuff), PSTR("\n%li,%.1f,%.1f,%i,%i"), data[cLine].timestampUTC, data[cLine].data.dht.temp, data[cLine].data.dht.humidity, data[cLine].data.mhz.temp, data[cLine].data.mhz.co2);
        if((nChars + rVal + 1) <= maxLen) {
          strcpy(cBPtr, tBuff);
          currentLine++;
          cBPtr += rVal;
          nChars += rVal;
        } else {
          break;
        }
      }
    }
    return nChars;
  });
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
  Serial.println("Done.");
}
