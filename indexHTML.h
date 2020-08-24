// The web page that displays the readings
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
  <font id="PageTitleFont" color="black">
    <span id="Uptime" title="Awaiting update"/>
    <h2 id="PageTitleText">Environmental monitor</h2>
  </font>
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
sessionStorage.setItem("connectionBroken", "false");
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
      if(sessionStorage.getItem("connectionBroken") == "true") {
        sessionStorage.setItem("connectionBroken", "false");
        document.getElementById("PageTitleFont").setAttribute("color", "black");
        document.getElementById("PageTitleText").innerHTML = "Environmental monitor";
      }
    }
  };
  xhttp.onerror = function() {
    if(sessionStorage.getItem("connectionBroken") == "false") {
      sessionStorage.setItem("connectionBroken", "true");
      document.getElementById("PageTitleFont").setAttribute("color", "red");
      document.getElementById("PageTitleText").innerHTML = "Environmental monitor<br/>(Not connected)";
    }
  }
  xhttp.open("GET", request, true);
  xhttp.send();
}
function checkCalibrate() {
  var rVal = confirm("Ensure the sensor has been in a 400ppm atmosphere (outside) for half an hour");
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
