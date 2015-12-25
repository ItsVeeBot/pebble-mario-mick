var initialized = false;
var options = {
  "config_show_no_phone": true,
  "config_show_weather": true,
  "config_temperature_units": 0,
  "config_show_battery": true,
  "config_show_phone_battery": false,
  "config_vibe": false,
  "config_vibe_hour": false,
  "config_background": 0,
};
var weatherAPI = "5d949dcb47bf776e783aa63ed22d4f60";

var locationOptions = { "timeout": 60000, "maximumAge": 60000 * 30 }; 

function locationSuccess(pos) {
  coordinates = pos.coords;
  console.log("Location: lat = "+coordinates.latitude+", lon = "+coordinates.longitude);
  console.log("Requesting weather...");
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?lat="+coordinates.latitude+"&lon="+coordinates.longitude+"&APPID="+weatherAPI, true);
  req.onreadystatechange = function() {
    if (req.readyState == 4) {
      if(req.status == 200) {
        //console.log("Response: "+req.responseText);
        var response = JSON.parse(req.responseText);				
        if (response.weather.length > 0)
        {
          var weather = response.weather[0];
          var temp = response.main.temp;
          //console.log("Icon: "+weather.icon);
          //console.log("Temperature: "+temp);
          var iconId = parseInt(weather.icon.substring(0,2));
          if (weather.icon.charAt(2) == "n") iconId += 100;
          var tempConverted = temp - 273.15;
          if (options.config_temperature_units == 0)
            tempConverted = Math.round(tempConverted*1.8 + 32);
          else
            tempConverted = Math.round(tempConverted);
          console.log("Icon id: "+iconId);
          console.log("Temperature: "+tempConverted);
          Pebble.sendAppMessage({"weather_icon_id":iconId, "weather_temperature": tempConverted});
        }
      } else {
        console.warn("HTTP error: "+req.status);
      }
    }
  }
  req.send(null);
}

function locationError() {
  console.warn("Can't detect location"); 
}

function weatherUpdate()
{
  console.log("Requesting location...");
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
}

Pebble.addEventListener("ready", function() {
  initialized = true;
	var json = window.localStorage.getItem('mario-config');	
	if (typeof json === 'string') {
		try {
      options = JSON.parse(json);
			Pebble.sendAppMessage(options);
			console.log("Loaded stored config: " + json);
    } catch(e) {
      console.log("stored config json parse error: " + json + ' - ' + e);
    }
  }
  //weatherUpdate();
});

Pebble.addEventListener("showConfiguration", function() {  
  var cfg = '?config=' + encodeURI(JSON.stringify(options));
  try
  {
    var watch = Pebble.getActiveWatchInfo();
  } catch(e) {
    console.log("getActiveWatchInfo error: " + e);
    var watch = null;
  }
  var platform = "&platform=" + ((watch != null) ? watch.platform : "unknown");
  var config_version = "&v=3";
  console.log("Showing configuration");
	Pebble.openURL("http://clusterrr.com/pebble_configs/mario_w.php" + cfg + platform + config_version);
});

Pebble.addEventListener("webviewclosed", function(e) {
	var response = decodeURIComponent(e.response);
  if (response.charAt(0) == "{" && response.slice(-1) == "}" && response.length > 5) {
    window.localStorage.setItem('mario-config', response);
		try {
			options = JSON.parse(response);
			Pebble.sendAppMessage(options);
      if (options.config_show_weather)
        setTimeout(weatherUpdate, 5000);
    } catch(e) {
			console.log("Response config json parse error: " + response + ' - ' + e);
		}
    console.log("Options = " + response);
  }
});

Pebble.addEventListener("appmessage",
  function(e) {
    console.log("Appmessage: "+JSON.stringify(e.payload));
		if ("weather_request" in e.payload)
		{
      setTimeout(weatherUpdate, 5000);
		}
  }
);
