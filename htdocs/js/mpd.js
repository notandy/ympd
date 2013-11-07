var socket;
var last_state;
var current_app;

$('#volumeslider').slider().on('slide', function(event) {
	socket.send("MPD_API_SET_VOLUME,"+event.value);
});

$('#nav_links > li').on('click', function(e) {
	//$('#nav_links > li:contains(' + History.getState().title + ')').removeClass('active');
	//History.pushState({state:$(this).attr('id')}, $(this).text(), '?' + $(this).attr('id'));
});

var app = $.sammy(function() {
	this.before('/', function(e, data) {
		socket.send("MPD_API_GET_TRACK_INFO");
	});

	this.get('#/', function() {
		current_app = 'playlist';
		$('#salamisandwich').find("tr:gt(0)").remove();
		socket.send("MPD_API_GET_PLAYLIST");
		$('#panel-heading').text("Playlist");
	});
	this.get(/\#\/browse\/(.*)/, function() {
		current_app = 'browse';
		$('#salamisandwich').find("tr:gt(0)").remove();
		var path = this.params['splat'];
		if(path == '')
			path = "/";

		socket.send("MPD_API_GET_BROWSE,"+path);
		$('#panel-heading').text("Browse database: "+path+"");
	});
	this.get('#/about', function() {
		current_app = 'about';
		$('#panel-heading').text("About");
	}); 
	this.get("/", function(context) {
		context.redirect("#/");
	});
});



/*
History.Adapter.bind(window,'statechange',function(){ // Note: We are using statechange instead of popstate

    if(!State.data.state) {
    	//$('#' + State.data).
    }

	$('#panel-heading').text(State.title);
	switch(State.data.state) {
		case "nav_browse":
			socket.send('MPD_API_GET_BROWSE,/');
			break;
		case "nav_about":
			break;
		case "nav_playlist":
		default:
			
	}
	$(this).addClass('active');

    console.log(" Browser State: ");
    console.log(State);
	socket.send("MPD_API_GET_TRACK_INFO");
});
*/
$(document).ready(function(){
	webSocketConnect();
});


function webSocketConnect() {

	if (typeof MozWebSocket != "undefined") {
		socket = new MozWebSocket(get_appropriate_ws_url(), "ympd-client");
	} else {
		socket = new WebSocket(get_appropriate_ws_url(), "ympd-client");
	}

	try {
		socket.onopen = function() {
			console.log("Connected");
			app.run();

			/* Push Initial state on first visit */
			//$(window).trigger("statechange")
		}

		socket.onmessage =function got_packet(msg) {
			if(msg.data === last_state)
				return;

			var obj = JSON.parse(msg.data);
			switch (obj.type) {
				case "playlist":
					//if(state.data.state !== 'nav_playlist')
					//	break;
					if(current_app !== 'playlist')
						break;

					for (var song in obj.data) {
						var minutes = Math.floor(obj.data[song].duration / 60);
						var seconds = obj.data[song].duration - minutes * 60;

						$('#salamisandwich tr:last').after(
							"<tr id=\"playlist_" + obj.data[song].id + "\"><td>" + obj.data[song].pos + "</td>" +
							"<td>"+ obj.data[song].title +"</td>" + 
							"<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
							"<span class=\"glyphicon glyphicon-trash pull-right hoverhidden\"></span> </td></tr>");
					}
					break;
				case "browse":
					//if(state.data.state !== 'nav_browse')
					//	break;
					if(current_app !== 'browse')
						break;
					console.log(app);

					for (var item in obj.data) {
						switch(obj.data[item].type) {
							case "directory":
								$('#salamisandwich tr:last').after(
									"<tr><td><span class=\"glyphicon glyphicon-folder-open\"></span></td>" + 
									"<td><a href=\"#/browse/"+ obj.data[item].dir +"\">" + obj.data[item].dir +"</a></td>" + 
									"<td></td></tr>");
								break;
							case "song":
								var minutes = Math.floor(obj.data[item].duration / 60);
								var seconds = obj.data[item].duration - minutes * 60;

								$('#salamisandwich tr:last').after(
									"<tr><td><span class=\"glyphicon glyphicon-music\"></span></td>" + 
									"<td><a href=\"#\">" + obj.data[item].title +"</a></td>" + 
									"<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +"</td></tr>");
								break;
							case "playlist":
								break;
						}
					}
					$('#salamisandwich td:eq(1)').click(function(){
						console.log($(this).children().attr("path"));
						//socket.send('MPD_API_GET_BROWSE,'+;
					});

					break;
				case "state":
					if(JSON.stringify(obj) === JSON.stringify(last_state))
						break;

					var total_minutes = Math.floor(obj.data.totalTime / 60);
					var total_seconds = obj.data.totalTime - total_minutes * 60;

					var elapsed_minutes = Math.floor(obj.data.elapsedTime / 60);
					var elapsed_seconds = obj.data.elapsedTime - elapsed_minutes * 60;

					$('#volumeslider').slider('setValue', obj.data.volume);
					var progress = Math.floor(100*obj.data.elapsedTime/obj.data.totalTime) + "%";
					$('#progressbar').width(progress);

					$('#counter')
						.text(elapsed_minutes + ":" + 
						(elapsed_seconds < 10 ? '0' : '') + elapsed_seconds + " / " +
						total_minutes + ":" + (total_seconds < 10 ? '0' : '') + total_seconds);

					$('#salamisandwich > thead  > tr').each(function(value) {
						$(this).removeClass('success');
					}); 
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

					if(obj.data.elapsedTime <= 1)
						socket.send("MPD_API_GET_TRACK_INFO");

					break;
				case "disconnected":
				    $('#alert')
				    .text("Server lost connection to MPD Host.")
				    .removeClass("hide alert-info")
				    .addClass("alert-danger");
                    break;

				case "current_song":
					$('#currenttrack').text(" " + obj.data.title);

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


$('#btnrandom').on('click', function (e) {
	socket.send("MPD_API_TOGGLE_RANDOM," + ($(this).hasClass('active') ? 0 : 1));

});
$('#btnconsume').on('click', function (e) {
	socket.send("MPD_API_TOGGLE_CONSUME," + ($(this).hasClass('active') ? 0 : 1));

});
$('#btnsingle').on('click', function (e) {
	socket.send("MPD_API_TOGGLE_SINGLE," + ($(this).hasClass('active') ? 0 : 1));
});
$('#btnrepeat').on('click', function (e) {
	socket.send("MPD_API_TOGGLE_REPEAT," + ($(this).hasClass('active') ? 0 : 1));
});
