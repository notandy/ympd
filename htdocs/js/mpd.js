var socket;
var last_state;
var current_app;
var is_firefox;

$('#volumeslider').slider().on('slide', function(event) {
	socket.send("MPD_API_SET_VOLUME,"+event.value);
});

var app = $.sammy(function() {
	this.before('/', function(e, data) {
		socket.send("MPD_API_GET_TRACK_INFO");
		$('#nav_links > li').removeClass('active');
	});

	this.get('#/', function() {
		current_app = 'playlist';
		$('#breadcrump').addClass('hide');
		$('#salamisandwich').find("tr:gt(0)").remove();
		if(is_firefox)
			$.get( "/api/get_playlist", socket.onmessage);
		else
			socket.send("MPD_API_GET_PLAYLIST");

		$('#panel-heading').text("Playlist");
		$('#playlist').addClass('active');
	});

	this.get(/\#\/browse\/(.*)/, function() {
		current_app = 'browse';
		$('#breadcrump').removeClass('hide').empty();
		$('#salamisandwich').find("tr:gt(0)").remove();
		var path = this.params['splat'];
		if(path == '')
			path = "/";

		if(is_firefox)
			$.get( "/api/get_browse/" + encodeURIComponent(path), socket.onmessage);
		else
			socket.send("MPD_API_GET_BROWSE,"+path);
		
		$('#panel-heading').text("Browse database: "+path+"");
		var path_array = path[0].split('/');
		var full_path = "";
		$.each(path_array, function(index, chunk) {
			if(path_array.length - 1 == index) {
				$('#breadcrump').append("<li class=\"active\">"+ chunk + "</li>");
				return;
			}

			full_path = full_path + chunk;
			$('#breadcrump').append("<li><a href=\"#/browse/" + full_path + "\">"+chunk+"</a></li>");
			full_path += "/";
		});
		$('#browse').addClass('active');
	});

	this.get("/", function(context) {
		context.redirect("#/");
	});
});

$(document).ready(function(){
	is_firefox = true;
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
			console.log(typeof msg);
			if(msg instanceof MessageEvent) {
				if(msg.data === last_state)
					return;

				var obj = JSON.parse(msg.data);
			} else {
				var obj = msg;
			}

			switch (obj.type) {
				case "playlist":
					if(current_app !== 'playlist')
						break;

					$('#salamisandwich > tbody').empty();
					for (var song in obj.data) {
						var minutes = Math.floor(obj.data[song].duration / 60);
						var seconds = obj.data[song].duration - minutes * 60;

						$('#salamisandwich > tbody').append(
							"<tr trackid=\"" + obj.data[song].id + "\"><td>" + obj.data[song].pos + "</td>" +
							"<td>"+ obj.data[song].title +"</td>" + 
							"<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
							"</td></tr>");
					}

					$('#salamisandwich > tbody > tr').on({
						mouseover: function(){
							if($(this).children().last().has("a").length == 0)
								$(this).children().last().append(
									"<a class=\"pull-right btn-group-hover\" href=\"#/\" " +
									"onclick=\"socket.send('MPD_API_RM_TRACK," + $(this).attr("trackid") +"'); $(this).parents('tr').remove();\">" +
									"<span class=\"glyphicon glyphicon-trash\"></span></a>")
								.find('a').fadeTo('fast',1);
						},
						click: function() {
							$('#salamisandwich > tbody > tr').removeClass('active');
							socket.send('MPD_API_PLAY_TRACK,'+$(this).attr('trackid'));
							$(this).addClass('active');
						},
						mouseleave: function(){
							$(this).children().last().find("a").stop().remove();
						}
					});

					break;
				case "browse":
					//if(state.data.state !== 'nav_browse')
					//	break;
					if(current_app !== 'browse')
						break;

					for (var item in obj.data) {
						switch(obj.data[item].type) {
							case "directory":
								$('#salamisandwich > tbody').append(
									"<tr uri=\"" + obj.data[item].dir + "\">" +
									"<td><span class=\"glyphicon glyphicon-folder-open\"></span></td>" + 
									"<td><a href=\"#/browse/"+ obj.data[item].dir +"\">" + basename(obj.data[item].dir) +"</a></td>" + 
									"<td></td></tr>");
								break;
							case "song":
								var minutes = Math.floor(obj.data[item].duration / 60);
								var seconds = obj.data[item].duration - minutes * 60;

								$('#salamisandwich > tbody').append(
									"<tr uri=\"" + obj.data[item].uri + "\">" +
									"<td><span class=\"glyphicon glyphicon-music\"></span></td>" + 
									"<td>" + obj.data[item].title +"</td>" + 
									"<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +"</td></tr>");
								break;

							case "playlist":
								break;
						}
					}

					$('#salamisandwich > tbody > tr').on({
						mouseover: function(){
							if($(this).children().last().has("a").length == 0)
								if($(this).attr("uri").length > 0)
									$(this).children().last().append(
										"<a role=\"button\" class=\"pull-right btn-group-hover\" " +
										"onclick=\"socket.send('MPD_API_ADD_TRACK," + $(this).attr("uri") +"'); $(this).parents('tr').addClass('active');\">" +
										"<span class=\"glyphicon glyphicon-plus\"></span></a>")
										.find('a').fadeTo('fast',1);
						},
						mouseleave: function(){
							$(this).children().last().find("a").stop().remove();
						}
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

					$('#salamisandwich > tbody > tr').removeClass('active').css("font-weight", "");
					$('#salamisandwich > tbody > tr[trackid='+obj.data.currentsongid+']').addClass('active').css("font-weight", "bold");

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

					$('#alert').addClass("hide");
					break;
				case "disconnected":
				    $('#alert')
				    .text("Server lost connection to MPD Host.")
				    .removeClass("hide alert-info")
				    .addClass("alert-danger");
                    break;

				case "current_song":
					$('#currenttrack').text(" " + obj.data.title);
					if(obj.data.album)
						$('#album').text(obj.data.album);
					if(obj.data.artist)
						$('#artist').text(obj.data.artist);
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

function basename(path) {
   return path.split('/').reverse()[0];
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
