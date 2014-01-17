var socket;
var last_state;
var current_app;
var current_song = new Object();

var app = $.sammy(function() {
    this.before('/', function(e, data) {
        $('#nav_links > li').removeClass('active');
    });

    this.get('#/', function() {
        current_app = 'playlist';
        $('#breadcrump').addClass('hide');
        $('#salamisandwich').find("tr:gt(0)").remove();
        $.get( "/api/get_playlist", socket.onmessage);

        $('#panel-heading').text("Playlist");
        $('#playlist').addClass('active');
    });

    this.get(/\#\/browse\/(.*)/, function() {
        current_app = 'browse';
        $('#breadcrump').removeClass('hide').empty().append("<li><a href=\"#/browse/\">root</a></li>");
        $('#salamisandwich').find("tr:gt(0)").remove();
        var path = this.params['splat'][0];

        $.get( "/api/get_browse/" + encodeURIComponent(path), socket.onmessage);

        $('#panel-heading').text("Browse database: "+path+"");
        var path_array = path.split('/');
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
    webSocketConnect();
    $("#volumeslider").slider(0);
    $("#volumeslider").on('slider.newValue', function(evt,data){
        socket.send("MPD_API_SET_VOLUME,"+data.val);
    });
    $('#progressbar').slider(0);
    $("#progressbar").on('slider.newValue', function(evt,data){
        if(current_song && current_song.currentSongId >= 0) {
            var seekVal = Math.ceil(current_song.totalTime*(data.val/100));
            socket.send("MPD_API_SET_SEEK,"+current_song.currentSongId+","+seekVal);
        }
    });
});


function webSocketConnect() {
    if (typeof MozWebSocket != "undefined") {
        socket = new MozWebSocket(get_appropriate_ws_url(), "ympd-client");
    } else {
        socket = new WebSocket(get_appropriate_ws_url(), "ympd-client");
    }

    try {
        socket.onopen = function() {
            $('.top-right').notify({
                message:{text:"Connected to ympd"},
                fadeOut: { enabled: true, delay: 500 }
            }).show();

            app.run();
        }

        socket.onmessage =function got_packet(msg) {
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
                            "<tr trackid=\"" + obj.data[song].id + "\"><td>" + (obj.data[song].pos + 1) + "</td>" +
                                "<td>"+ obj.data[song].title +"</td>" + 
                                "<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
                        "</td><td></td></tr>");
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
                    if(current_app !== 'browse')
                        break;

                    for (var item in obj.data) {
                        switch(obj.data[item].type) {
                            case "directory":
                                $('#salamisandwich > tbody').append(
                                    "<tr uri=\"" + obj.data[item].dir + "\" class=\"dir\">" +
                                        "<td><span class=\"glyphicon glyphicon-folder-open\"></span></td>" + 
                                        "<td><a>" + basename(obj.data[item].dir) + "</a></td>" + 
                                "<td></td><td></td></tr>");
                            break;
                        case "song":
                            var minutes = Math.floor(obj.data[item].duration / 60);
                            var seconds = obj.data[item].duration - minutes * 60;

                            $('#salamisandwich > tbody').append(
                                "<tr uri=\"" + obj.data[item].uri + "\" class=\"song\">" +
                                    "<td><span class=\"glyphicon glyphicon-music\"></span></td>" + 
                                    "<td>" + obj.data[item].title +"</td>" + 
                                    "<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +"</td><td></td></tr>");
                                break;

                            case "playlist":
                                break;
                        }
                    }

                    function appendClickableIcon(appendTo, onClickAction, glyphicon) {
                        $(appendTo).children().last().append(
                            "<a role=\"button\" class=\"pull-right btn-group-hover\">" +
                            "<span class=\"glyphicon glyphicon-" + glyphicon + "\"></span></a>")
                            .find('a').click(function(e) {
                                e.stopPropagation();
                                socket.send(onClickAction + "," + $(this).parents("tr").attr("uri"));
                            $('.top-right').notify({
                                message:{
                                    text: $('td:nth-child(2)', $(this).parents("tr")).text() + " added"
                                } }).show();
                            }).fadeTo('fast',1);
                    }

                    $('#salamisandwich > tbody > tr').on({
                        mouseenter: function() {
                            if($(this).is(".dir")) 
                                appendClickableIcon($(this), 'MPD_API_ADD_TRACK', 'plus');
                            else if($(this).is(".song"))
                                appendClickableIcon($(this), 'MPD_API_ADD_PLAY_TRACK', 'play');
                        },
                        click: function() {
                            if($(this).is(".song")) {
                                socket.send("MPD_API_ADD_TRACK," + $(this).attr("uri"));
                                $('.top-right').notify({
                                    message:{
                                        text: $('td:nth-child(2)', this).text() + " added"
                                    }
                                }).show();

                            } else
                                app.setLocation("#/browse/"+$(this).attr("uri"));
                        },
                        mouseleave: function(){
                            $(this).children().last().find("a").stop().remove();
                        }
                    });

                    break;
                case "state":
                    updatePlayIcon(obj.data.state);
                    updateVolumeIcon(obj.data.volume);

                    if(JSON.stringify(obj) === JSON.stringify(last_state))
                        break;

                    current_song.totalTime  = obj.data.totalTime;
                    current_song.currentSongId = obj.data.currentsongid;
                    var total_minutes = Math.floor(obj.data.totalTime / 60);
                    var total_seconds = obj.data.totalTime - total_minutes * 60;

                    var elapsed_minutes = Math.floor(obj.data.elapsedTime / 60);
                    var elapsed_seconds = obj.data.elapsedTime - elapsed_minutes * 60;

                    $('#volumeslider').slider(obj.data.volume);
                    var progress = Math.floor(100*obj.data.elapsedTime/obj.data.totalTime);
                    $('#progressbar').slider(progress);

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

                    last_state = obj;
                    break;
                case "disconnected":
                    if($('.top-right').has('div').length == 0)
                        $('.top-right').notify({
                            message:{text:"ympd lost connection to MPD "},
                            type: "danger",
                            fadeOut: { enabled: true, delay: 1000 },
                        }).show();
                    break;
                case "update_playlist":
                    if(current_app === 'playlist')
                        $.get( "/api/get_playlist", socket.onmessage);
                    break;
                case "song_change":
                    $('#currenttrack').text(" " + obj.data.title);
                    var notification = "<strong><h4>" + obj.data.title + "</h4></strong>";

                    if(obj.data.album) {
                        $('#album').text(obj.data.album);
                        notification += obj.data.album + "<br />";
                    }
                    if(obj.data.artist) {
                        $('#artist').text(obj.data.artist);
                        notification += obj.data.artist + "<br />";
                    }

                    $('.top-right').notify({
                        message:{html: notification},
                        type: "info",
                    }).show();
                    break;
                case "error":
                    $('.top-right').notify({
                        message:{text: obj.data},
                        type: "danger",
                    }).show();
                default:
                    break;
            }


        }
        socket.onclose = function(){
            console.log("Disconnected");
            $('.top-right').notify({
                message:{text:"Connection to ympd lost, retrying in 3 seconds "},
                type: "danger", 
                onClose: function () {
                    webSocketConnect();
                }
            }).show();
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
    .removeClass("glyphicon-pause");
    $('#track-icon').removeClass("glyphicon-play")
    .removeClass("glyphicon-pause")
    .removeClass("glyphicon-stop");

    if(state == 1) { // stop
        $("#play-icon").addClass("glyphicon-play");
        $('#track-icon').addClass("glyphicon-stop");
    } else if(state == 2) { // pause
        $("#play-icon").addClass("glyphicon-pause");
        $('#track-icon').addClass("glyphicon-play");
    } else { // play
        $("#play-icon").addClass("glyphicon-play");
        $('#track-icon').addClass("glyphicon-pause");
    }
}

function updateDB() {
    socket.send('MPD_API_UPDATE_DB');
    $('.top-right').notify({
        message:{text:"Updating MPD Database... "}
    }).show();
}

function clickPlay() {
    if($('#track-icon').hasClass('glyphicon-stop'))
        socket.send('MPD_API_SET_PLAY');
    else
        socket.send('MPD_API_SET_PAUSE');
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

function getVersion()
{
    $.get( "/api/get_version", function(response) {
        $('#ympd_version').text(response.data.ympd_version);
        $('#mpd_version').text(response.data.mpd_version);
    });
}