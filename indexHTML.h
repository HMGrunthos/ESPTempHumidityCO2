// The web page that displays the readings
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <script src="https://d3js.org/d3.v6.min.js"></script>
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
    .label {
      font-size: 1.5rem;
      color: black;
    }
    .value {
      font-weight: bold;
    }
    .units {
      font-size: 0.9rem;
      color: black;
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
  <h2 id="PageTitleText" style="color:black">Environmental monitor</h2>
  <span id="CO2Age" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-wind" style="color:#05ee8a;"></i>
      <span class="label">Carbon dioxide</span>
      <span id="co2" class="value" style="color:red">nan</span>
      <sup class="units">ppm</sup>
    </p>
  </div>
  <span id="RHAge" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-tint" style="color:#00add6;"></i>
      <span class="label">Humidity</span>
      <span id="humidity" class="value" style="color:red">nan</span>
      <sup class="units">&percnt;</sup>
    </p>
  </div>
  <span id="TempMHZAge" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a"></i>
      <span class="label">Temperature<sub class="units">MHZ<sub/></span>
      <span id="tempMHZ" class="value" style="color:red">nan</span>
      <sup class="units">&deg;C</sup>
    </p>
  </div>
  <span id="TempDHTAge" title="Awaiting update"/>
  <div class="center">
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a"></i>
      <span class="label">Temperature<sub class="units">DHT<sub/></span>
      <span id="tempDHT" class="value" style="color:red">nan</span>
      <sup class="units">&deg;C</sup>
    </p>
  </div>
  <div id="dataviz_brushZoom"></div>
  <div id="footer">
    <button onclick="checkCalibrate()">Calibrate CO2 sensor</button>
    <button onclick="checkDisableCalibrate()">Disable automatic calibration</button>
    <button onclick="checkEnableCalibrate()">Enable automatic calibration</button>
  </div>
  <script>
    // set the dimensions and margins of the graph
    var margin = {top: 10, right: 30, bottom: 30, left: 60},
        width = 460 - margin.left - margin.right,
        height = 400 - margin.top - margin.bottom;
    
    // append the svg object to the body of the page
    var Svg = d3.select("#dataviz_brushZoom")
      .append("svg")
        .attr("width", width + margin.left + margin.right)
        .attr("height", height + margin.top + margin.bottom)
      .append("g")
        .attr("transform",
              "translate(" + margin.left + "," + margin.top + ")");
    
    //Read the data
    d3.csv("https://raw.githubusercontent.com/holtzy/D3-graph-gallery/master/DATA/iris.csv", function(data) {
      // Add X axis
      var x = d3.scaleLinear()
        .domain([4, 8])
        .range([ 0, width ]);
      var xAxis = Svg.append("g")
        .attr("transform", "translate(0," + height + ")")
        .call(d3.axisBottom(x));
    
      // Add Y axis
      var y = d3.scaleLinear()
        .domain([0, 70])
        .range([ height, 0]);
      Svg.append("g")
        .call(d3.axisLeft(y));
    
      // Add a clipPath: everything out of this area won't be drawn.
      var clip = Svg.append("defs").append("svg:clipPath")
          .attr("id", "clip")
          .append("svg:rect")
          .attr("width", width )
          .attr("height", height )
          .attr("x", 0)
          .attr("y", 0);
    
      // Color scale: give me a specie name, I return a color
      var color = d3.scaleOrdinal()
        .domain(["setosa", "versicolor", "virginica" ])
        .range([ "#440154ff", "#21908dff", "#fde725ff"])
    
      // Add brushing
      var brush = d3.brushX()                 // Add the brush feature using the d3.brush function
          .extent( [ [0,0], [width,height] ] ) // initialise the brush area: start at 0,0 and finishes at width,height: it means I select the whole graph area
          .on("end", updateChart) // Each time the brush selection changes, trigger the 'updateChart' function
    
      // Create the scatter variable: where both the circles and the brush take place
      var scatter = Svg.append('g')
        .attr("clip-path", "url(#clip)")

		scatter
		  .append("path")
        .datum(data)
        .attr("fill", "none")
        .attr("stroke", "#69b3a2")
        .attr("stroke-width", 1.5)
        .attr("d", d3.line()
          .x(function(d) { return x(d.Sepal_Length) })
          .y(function(d) { return y(d.Petal_Length) })
        )

      // Add the points
      scatter
        .selectAll("dot")
        .data(data)
        .enter()
        .append("circle")
          .attr("cx", function(d) { return x(d.Sepal_Length) } )
          .attr("cy", function(d) { return y(d.Petal_Length) } )
          .attr("r", 2)
          .attr("fill", "#000000")
    
      // Add the brushing
      scatter
        .append("g")
          .attr("class", "brush")
          .call(brush);
    
      // A function that set idleTimeOut to null
      var idleTimeout
      function idled() { idleTimeout = null; }
    
      // A function that update the chart for given boundaries
      function updateChart() {
        extent = d3.event.selection
    
        // If no selection, back to initial coordinate. Otherwise, update X axis domain
        if(!extent){
          if (!idleTimeout) return idleTimeout = setTimeout(idled, 350); // This allows to wait a little bit
          x.domain([ 4,8])
        }else{
          x.domain([ x.invert(extent[0]), x.invert(extent[1]) ])
          scatter.select(".brush").call(brush.move, null) // This remove the grey brush area as soon as the selection has been done
        }
    
        // Update axis and circle position
        xAxis.transition().duration(1000).call(d3.axisBottom(x))
        scatter
          .select("path")
          .transition().duration(1000)
			 .attr("d", d3.line()
            .x(function(d) { return x(d.Sepal_Length) })
            .y(function(d) { return y(d.Petal_Length) })
          )

        scatter
          .selectAll("circle")
          .transition().duration(1000)
          .attr("cx", function (d) { return x(d.Sepal_Length); } )
          .attr("cy", function (d) { return y(d.Petal_Length); } )
      }
    }
  )
  </script>
</body>
<script>
sessionStorage.setItem("connectionBroken", "false");
function updateFields(request, field, mouseText, mouseOverComment) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var res = this.responseText.split(",");
      var mouseOverText;
      var textColour;
      if(res[1] == 0) {
        mouseOverText = "Never updated";
        textColour = "color:red";
      } else {
        mouseOverText = (res[1]/1000).toFixed(0) + mouseOverComment;
        textColour = "color:black";
      }
      document.getElementById(mouseText).setAttribute("title", mouseOverText);
      if(field != "") {
        document.getElementById(field).innerHTML = res[0];
        document.getElementById(field).setAttribute("style", textColour);
      }
      if(sessionStorage.getItem("connectionBroken") == "true") {
        sessionStorage.setItem("connectionBroken", "false");
        document.getElementById("PageTitleText").setAttribute("style", "color:black");
        document.getElementById("PageTitleText").innerHTML = "Environmental monitor";
      }
    }
  };
  xhttp.onerror = function() {
    if(sessionStorage.getItem("connectionBroken") == "false") {
      sessionStorage.setItem("connectionBroken", "true");
      document.getElementById("PageTitleText").setAttribute("style", "color:red");
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
function checkDisableCalibrate() {
  var rVal = confirm("Are you sure you wish to disable automatic backgroud calibration");
  if (rVal == true) {
    var calRequest = new XMLHttpRequest();
    calRequest.open("GET", "/disableCO2ABC", false);
    calRequest.send();
  }
}
function checkEnableCalibrate() {
  var rVal = confirm("Are you sure you wish to enable automatic backgroud calibration");
  if (rVal == true) {
    var calRequest = new XMLHttpRequest();
    calRequest.open("GET", "/enableCO2ABC", false);
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
