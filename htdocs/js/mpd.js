var socket;

if (typeof MozWebSocket != "undefined") {
	socket = new MozWebSocket(get_appropriate_ws_url(), "ympd-client");
} else {
	socket = new WebSocket(get_appropriate_ws_url(), "ympd-client");
}


try {
	socket.onopen = function() {
		console.log("Connected");
	} 

	socket.onmessage =function got_packet(msg) {
		console.log(msg.data);
	} 

	socket.onclose = function(){
		console.log("Disconnected");

	}
} catch(exception) {
	alert('<p>Error' + exception);  
}

function get_appropriate_ws_url()
{
	var pcol;
	var u = document.URL;

	/*
	 * We open the websocket encrypted if this page came on an
	 * https:// url itself, otherwise unencrypted
	 */

	if (u.substring(0, 5) == "https") {
		pcol = "wss://";
		u = u.substr(8);
	} else {
		pcol = "ws://";
		if (u.substring(0, 4) == "http")
			u = u.substr(7);
	}

	u = u.split('/');

	return pcol + u[0];
}

var updateVolumeIcon = function(mute)
{
	$("#volume-icon").removeClass("glyphicon-volume-off");
	$("#volume-icon").removeClass("glyphicon-volume-up");

	if(mute) {
		$("#volume-icon").addClass("glyphicon-volume-off");
		$("#volume").button('toggle')
	} else {
		$("#volume-icon").addClass("glyphicon-volume-up");
		$("#volume").button('reset')
	}
}

function test() {
	socket.send("MPD_API_GET_PLAYLIST");
}

