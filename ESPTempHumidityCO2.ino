/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <ESPAsyncWebServer.h>
#include <DHTesp.h>

#include "MHZ.h"

#ifdef ESP32
  #pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
  #error Select ESP8266 board.
#endif

// pin for uart reading
#define MH_Z14_RX D7
#define MH_Z14_TX D6

MHZ co2(MH_Z14_RX, MH_Z14_TX, MHZ14A);

// Replace with your network credentials
const char *ssid = "Sleeper cell - 2.4GHz";
const char *password = "ccec37ffd6";

#define DHTPIN 5     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT22

DHTesp dht;

// Current temperature, humidity and CO2
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

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
     white-space: nowrap;
    }
    h2 {
      font-size: 2rem;
    }
    .center {
      height: 50px;
      position: relative;
    }
    .center p {
      font-size: 1.75rem;
      margin: 0;
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
    }
    .units {
      font-size: 0.9rem;
    }
    #footer {
     position:absolute;
     bottom:0;
     width:100%;
     height:50px;
   }
  </style>
</head>
<body onload="updateAll()">
  <span id="Uptime" title="Awaiting update"/>
  <h2>Environmental monitor</h2>
  <span id="CO2Age" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-wind" style="color:#05ee8a;"></i>
      <span style="font-size: 1.5rem;">Carbon dioxide</span>
      <span id="co2" style="font-weight: bold;">nan</span>
      <sup class="units">ppm</sup>
    </p>
  </div>
  <span id="RHAge" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-tint" style="color:#00add6;"></i>
      <span style="font-size: 1.5rem;">Humidity</span>
      <span id="humidity" style="font-weight: bold;">nan</span>
      <sup class="units">&percnt;</sup>
    </p>
  </div>
  <span id="TempMHZAge" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a"></i>
      <span style="font-size: 1.5rem;">Temperature<sub class="units">MHZ<sub/></span>
      <span id="tempMHZ" style="font-weight: bold;">nan</span>
      <sup class="units">&deg;C</sup>
    </p>
  </div>
  <span id="TempDHTAge" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a"></i>
      <span style="font-size: 1.5rem;">Temperature<sub class="units">DHT<sub/></span>
      <span id="tempDHT" style="font-weight: bold;">nan</span>
      <sup class="units">&deg;C</sup>
    </p>
  </div>
  <div id="footer">
    <button onclick="checkCalibrate()">Calibrate CO2 sensor</button>
  </div>
</body>
<script>
function updateFields(request, field, mouseText, mouseOverComment) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var res = this.responseText.split(",");
      if(field != "") {
        document.getElementById(field).innerHTML = res[0];
      }
      var mouseOverText;
      if(res[1] == 0) {
        mouseOverText = "Never updated";
      } else {
        mouseOverText = (res[1]/1000).toFixed(0) + mouseOverComment;
      }
      document.getElementById(mouseText).setAttribute("title", mouseOverText);
    }
  };
  xhttp.open("GET", request, true);
  xhttp.send();
}
function checkCalibrate() {
  var rVal = confirm("Ensure the sensor has been in a 400ppm atmosphere for half an hour");
  if (rVal == true) {
    var calRequest = new XMLHttpRequest();
    calRequest.open("GET", "/calibrateCO2", false);
    calRequest.send();
  }
}
function updateTempDHT() {
  updateFields("/tempDHT", "tempDHT", "TempDHTAge", " seconds since last update");
}
function updateTempMHZ() {
  updateFields("/tempMHZ", "tempMHZ", "TempMHZAge", " seconds since last update");
}
function updateHumidity() {
  updateFields("/humidity", "humidity", "RHAge", " seconds since last update");
}
function updateCO2() {
  updateFields("/co2", "co2", "CO2Age", " seconds since last update");
}
function updateUptime() {
  updateFields("/uptime", "", "Uptime", " seconds total uptime");
}
function updateAll() {
  updateTempDHT();
  updateTempMHZ();
  updateHumidity();
  updateCO2();
}
setInterval(updateTempDHT, 7993);
setInterval(updateTempMHZ, 8999);
setInterval(updateHumidity, 9973);
setInterval(updateCO2, 10993);
setInterval(updateUptime, 11987);
</script>
</html>)rawliteral";

static String requestHandler(const String &var)
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

void setup()
{
  memset(&currentReadings, 0, sizeof(currentReadings));
  currentReadings.dht.temp.reading = NAN;
  currentReadings.dht.rh.reading = NAN;
  currentReadings.mhz.temp = NAN;
  currentReadings.mhz.co2 = NAN;

  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.setup(17, DHTesp::DHT22); // Connect DHT sensor to GPIO 17

  delay(100);

  co2.setDebug(true);

  // Connect to Wi-Fi
  WiFi.hostname("EnvMonOne");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/tempDHT", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", requestHandler("TEMPDHT").c_str());
  });
  server.on("/tempMHZ", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", requestHandler("TEMPMHZ").c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", requestHandler("HUMIDITY").c_str());
  });
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", requestHandler("CO2").c_str());
  });
  server.on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", requestHandler("UPTIME").c_str());
  });
  server.on("/calibrateCO2", HTTP_GET, [](AsyncWebServerRequest *request){
    co2.calibrate();
    request->send_P(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
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
