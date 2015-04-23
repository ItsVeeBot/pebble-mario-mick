var initialized = false;
var options = {
  "config_show_no_phone": true,
  "config_show_battery": true,
  "config_show_phone_battery": true,
  "config_vibe": false,
  "config_background": 0,
};

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
});

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
	var cfg = '?config=' + encodeURI(JSON.stringify(options));
  var watch = Pebble.getActiveWatchInfo();
  var platform = "&platform=" + ((watch != null) ? watch.platform : "unknown");
  Pebble.openURL("http://clusterrr.com/pebble_configs/mario.php" + cfg + platform);
});

Pebble.addEventListener("webviewclosed", function(e) {
	var response = decodeURIComponent(e.response);
  if (response.charAt(0) == "{" && response.slice(-1) == "}" && response.length > 5) {
    window.localStorage.setItem('mario-config', response);
		try {
			options = JSON.parse(response);
			Pebble.sendAppMessage(options);
    } catch(e) {
			console.log("Response config json parse error: " + response + ' - ' + e);
		}
    console.log("Options = " + response);
  }
});
