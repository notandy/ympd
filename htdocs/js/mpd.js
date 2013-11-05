var socket;
var last_state;

$('#volumeslider').slider().on('slide', function(event) {
	socket.send("MPD_API_SET_VOLUME,"+event.value);
});

webSocketConnect();

function webSocketConnect() {

	if (typeof MozWebSocket != "undefined") {
		socket = new MozWebSocket(get_appropriate_ws_url(), "ympd-client");
	} else {
		socket = new WebSocket(get_appropriate_ws_url(), "ympd-client");
	}

	try {
		socket.onopen = function() {
			console.log("Connected");
			socket.send("MPD_API_GET_PLAYLIST");
			socket.send("MPD_API_GET_STATE");
		}

		socket.onmessage =function got_packet(msg) {
			console.log(msg.data);
			if(msg.data === last_state)
				return;

			var obj = JSON.parse(msg.data);
			switch (obj.type) {
				case "playlist":
					for (var song in obj.data) {
						var minutes = Math.floor(obj.data[song].duration / 60);
						var seconds = obj.data[song].duration - minutes * 60;

						$('#salamisandwich tr:last').after(
							"<tr id=\"playlist_" + obj.data[song].id + "\"><td>" + obj.data[song].id + "</td>" +
							"<td>"+ obj.data[song].uri +"</td>" +
							"<td>"+ obj.data[song].title.replace(/%07/g, '"') +"</td>" + 
							"<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +"</td></tr>");
					}
					break;
				case "state":
					if(JSON.stringify(obj) === JSON.stringify(last_state))
						break;

					$('#volumeslider').slider('setValue', obj.data.volume);
					var progress = Math.floor(100*obj.data.elapsedTime/obj.data.totalTime) + "%";
					$('#progressbar').width(progress);
					$('#playlist_'+obj.data.currentsongid).addClass('success');
					if(obj.data.random)
						$('#btnrandom').addClass("active")
					else
						$('#btnrandom').removeClass("active");

					if(obj.data.consume)
						$('#btnconsume').addClass("active")
					else
						$('#btnconsume').removeClass("active");

					if(obj.data.single)
						$('#btnsingle').addClass("active")
					else
						$('#btnsingle').removeClass("active");

					if(obj.data.repeat)
						$('#btnrepeat').addClass("active")
					else
						$('#btnrepeat').removeClass("active");

					if(last_state && (obj.data.state !== last_state.data.state))
						updatePlayIcon(obj.data.state);
					if(last_state && (obj.data.volume !== last_state.data.volume))
						updateVolumeIcon(obj.data.volume);
					break;
				case "disconnected":
					$('#alert').text("Error: Connection to MPD failed.");
					$('#alert').removeClass("hide alert-info").addclass("alert-danger");
					break;

				default:
					break;
			}

			last_state = obj;

		}
		socket.onclose = function(){
			console.log("Disconnected");

			var seconds = 5;
			var tm = setInterval(disconnectAlert,1000);

			function disconnectAlert() {
				$('#alert')
				.text("Connection to MPD lost, retrying in " + seconds + " seconds")
				.removeClass("hide alert-info")
				.addClass("alert-danger");

				if(seconds-- <= 0) {
					webSocketConnect();
					seconds = 5;
					$('#alert').addClass("hide");
					clearInterval(tm);
				}
			}
			
		}

	} catch(exception) {
		alert('<p>Error' + exception);
	}

}

function get_appropriate_ws_url()
{
	var pcol;
	var u = document.URL;

	/*
	/* We open the websocket encrypted if this page came on an
	/* https:// url itself, otherwise unencrypted
	/*/

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

var updateVolumeIcon = function(volume)
{
	$("#volume-icon").removeClass("glyphicon-volume-off");
	$("#volume-icon").removeClass("glyphicon-volume-up");
	$("#volume-icon").removeClass("glyphicon-volume-down");

	if(volume == 0) {
		$("#volume-icon").addClass("glyphicon-volume-off");
	} else if (volume < 50) {
		$("#volume-icon").addClass("glyphicon-volume-down");
	} else {
		$("#volume-icon").addClass("glyphicon-volume-up");
	}
}

var updatePlayIcon = function(state)
{
	$("#play-icon").removeClass("glyphicon-play")
	.removeClass("glyphicon-pause")
	.removeClass("glyphicon-stop");
	$('#track-icon').removeClass("glyphicon-play")
	.removeClass("glyphicon-pause")
	.removeClass("glyphicon-stop");
	
	if(state == 1) {
		$("#play-icon").addClass("glyphicon-stop");
		$('#track-icon').addClass("glyphicon-stop");
	} else if(state == 2) {
		$("#play-icon").addClass("glyphicon-pause");
		$('#track-icon').addClass("glyphicon-play");
	} else {
		$("#play-icon").addClass("glyphicon-play");
		$('#track-icon').addClass("glyphicon-pause");
	}
}

function updateDB()
{
	socket.send('MPD_API_UPDATE_DB');

	$('#alert')
	.text("Updating MPD Database...")
	.removeClass("hide alert-danger")
	.addClass("alert-info");

	setTimeout(function() {
		$('#alert').addClass("hide");
	}, 5000);
}

function toggleButton(id) {
	switch (obj.type) {
		case "btnrandom":
			socket.send("MPD_API_TOGGLE_RANDOM");
			break;
		case "btnconsume":
			socket.send("MPD_API_TOGGLE_CONSUME");
			break;
		case "btnsingle":
			socket.send("MPD_API_TOGGLE_SINGLE");
			break;
		case "btnrepeat":
			socket.send("MPD_API_TOGGLE_REPEAT");
			break;
		default:
			break;
	}
}
