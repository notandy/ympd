/* ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

var socket;
var last_state;
var last_outputs;
var current_app;
var pagination = 0;
var browsepath;
var lastSongTitle = "";
var current_song = new Object();
var MAX_ELEMENTS_PER_PAGE = 512;
var dirble_selected_cat = "";
var dirble_catid = "";
var dirble_page = 1;
var isTouch = Modernizr.touch ? 1 : 0;

var app = $.sammy(function() {

    function runBrowse() {
        current_app = 'queue';

        $('#breadcrump').addClass('hide');
        $('#salamisandwich').removeClass('hide').find("tr:gt(0)").remove();
        socket.send('MPD_API_GET_QUEUE,'+pagination);

        $('#panel-heading').text("Queue");
        $('#queue').addClass('active');
    }

    function prepare() {
        $('#nav_links > li').removeClass('active');
        $('.page-btn').addClass('hide');
        $('#add-all-songs').hide();
        $('#dirble_panel').addClass('hide');
        $('#mpdScheduler_panel').addClass('hide');
        pagination = 0;
        browsepath = '';
    }

    this.get(/\#\/(\d+)/, function() {
        prepare();
        pagination = parseInt(this.params['splat'][0]);
        runBrowse();
    });

    this.get(/\#\/browse\/(\d+)\/(.*)/, function() {
        prepare();
        browsepath = this.params['splat'][1];
        pagination = parseInt(this.params['splat'][0]);
        current_app = 'browse';
        $('#breadcrump').removeClass('hide').empty().append("<li><a href=\"#/browse/0/\">root</a></li>");
        $('#salamisandwich').removeClass('hide').find("tr:gt(0)").remove();
        $('#dirble_panel').addClass('hide');
        socket.send('MPD_API_GET_BROWSE,'+pagination+','+(browsepath ? browsepath : "/"));
        // Don't add all songs from root
        if (browsepath) {
            var add_all_songs = $('#add-all-songs');
            add_all_songs.off(); // remove previous binds
            add_all_songs.on('click', function() {
                socket.send('MPD_API_ADD_TRACK,'+browsepath);
            });
            add_all_songs.show();
        }

        $('#panel-heading').text("Browse database: "+browsepath);
        var path_array = browsepath.split('/');
        var full_path = "";
        $.each(path_array, function(index, chunk) {
            if(path_array.length - 1 == index) {
                $('#breadcrump').append("<li class=\"active\">"+ chunk + "</li>");
                return;
            }

            full_path = full_path + chunk;
            $('#breadcrump').append("<li><a href=\"#/browse/0/" + full_path + "\">"+chunk+"</a></li>");
            full_path += "/";
        });
        $('#browse').addClass('active');
    });

    this.get(/\#\/search\/(.*)/, function() {
        current_app = 'search';
        $('#salamisandwich').find("tr:gt(0)").remove();
        $('#dirble_panel').addClass('hide');
        var searchstr = this.params['splat'][0];

        $('#search > div > input').val(searchstr);
        socket.send('MPD_API_SEARCH,' + searchstr);

        $('#panel-heading').text("Search: "+searchstr);
    });


    this.get(/\#\/dirble\/(\d+)\/(\d+)/, function() {
        prepare();
        current_app = 'dirble';
        $('#breadcrump').removeClass('hide').empty().append("<li><a href=\"#/dirble/\">Categories</a></li><li>"+dirble_selected_cat+"</li>");
        $('#salamisandwich').addClass('hide');
        $('#dirble_panel').removeClass('hide');
        $('#dirble_loading').removeClass('hide');
        $('#dirble_left').find("tr:gt(0)").remove();
        $('#dirble_right').find("tr:gt(0)").remove();

        $('#panel-heading').text("Dirble");
        $('#dirble').addClass('active');

        $('#next').addClass('hide');

        if (this.params['splat'][1] > 1) $('#prev').removeClass('hide');
        else $('#prev').addClass('hide');

        dirble_catid = this.params['splat'][0];
        dirble_page = this.params['splat'][1];

        dirble_load_stations();
    });


    this.get(/\#\/dirble\//, function() {
        prepare();
        current_app = 'dirble';
        $('#breadcrump').removeClass('hide').empty().append("<li>Categories</li>");
        $('#salamisandwich').addClass('hide');
        $('#dirble_panel').removeClass('hide');
        $('#dirble_loading').removeClass('hide');
        $('#dirble_left').find("tr:gt(0)").remove();
        $('#dirble_right').find("tr:gt(0)").remove();

        $('#panel-heading').text("Dirble");
        $('#dirble').addClass('active');

        dirble_load_categories();
    });

    this.get(/\#\/mpdScheduler\//, function() {
        prepare();
        current_app = 'mpdScheduler';
        $('#breadcrump').addClass('hide');
        $('#salamisandwich').addClass('hide');
        $('#mpdScheduler_panel').removeClass('hide');

        $('#panel-heading').text("Alarms");
        $('#mpdScheduler').addClass('active');

        socket.send('MPD_API_SCHEDULE_LIST');

        $('#mpdScheduler_addAlarm form').on('submit', function () {
            var time = $('#mpdScheduler_addAlarm_time').val().split(':').map(function (num) {
                return parseInt(num);
            });

            if (time.length == 2 && time[0] < 24 && time[1] < 60) {
                var hours = time[0];
                var mins = time[1];

                $("#mpdScheduler_addAlarm .form-group").removeClass('has-error');
                $('#mpdScheduler_addAlarm_time').val('');

                socket.send('MPD_API_SCHEDULE_ALARM,' + hours + ',' + mins);
                socket.send('MPD_API_SCHEDULE_LIST');
            } else {
                $("#mpdScheduler_addAlarm .form-group").addClass('has-error');
            }

            return false;
        });

        $('#mpdScheduler_addSleep button').on('click', function () {
            var mins = parseInt($(this).data('minutes'));

            // This will break if local time not equal to server time (e.g. different timezone)
            var alarmTime = new Date(Date.now() + mins * 60000);

            socket.send('MPD_API_SCHEDULE_SLEEP,' + alarmTime.getHours() + ',' + alarmTime.getMinutes());
            socket.send('MPD_API_SCHEDULE_LIST');

            return false;
        });
    });

    this.get("/", function(context) {
        context.redirect("#/0");
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

    $('#addstream').on('shown.bs.modal', function () {
        $('#streamurl').focus();
     })
    $('#addstream form').on('submit', function (e) {
        addStream();
    });

    if(!notificationsSupported())
        $('#btnnotify').addClass("disabled");
    else
        if ($.cookie("notification") === "true")
            $('#btnnotify').addClass("active")
});


function webSocketConnect() {
    if (typeof MozWebSocket != "undefined") {
        socket = new MozWebSocket(get_appropriate_ws_url());
    } else {
        socket = new WebSocket(get_appropriate_ws_url());
    }

    try {
        socket.onopen = function() {
            console.log("connected");
            $('.top-right').notify({
                message:{text:"Connected to ympd"},
                fadeOut: { enabled: true, delay: 500 }
            }).show();

            app.run();
            /* emit initial request for output names */
            socket.send("MPD_API_GET_OUTPUTS");
        }

        socket.onmessage = function got_packet(msg) {
            if(msg.data === last_state || msg.data.length == 0)
                return;

            var obj = JSON.parse(msg.data);

            switch (obj.type) {
                case "queue":
                    if(current_app !== 'queue')
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

                    if(obj.data[obj.data.length-1].pos + 1 >= pagination + MAX_ELEMENTS_PER_PAGE)
                        $('#next').removeClass('hide');
                    if(pagination > 0)
                        $('#prev').removeClass('hide');
                    if ( isTouch ) {
                        $('#salamisandwich > tbody > tr > td:last-child').append(
                                    "<a class=\"pull-right btn-group-hover\" href=\"#/\" " +
                                        "onclick=\"socket.send('MPD_API_RM_TRACK,' + $(this).parents('tr').attr('trackid')); $(this).parents('tr').remove();\">" +
                                "<span class=\"glyphicon glyphicon-trash\"></span></a>");
                    } else {
                        $('#salamisandwich > tbody > tr').on({
                            mouseover: function(){
                                if($(this).children().last().has("a").length == 0)
                                    $(this).children().last().append(
                                        "<a class=\"pull-right btn-group-hover\" href=\"#/\" " +
                                            "onclick=\"socket.send('MPD_API_RM_TRACK," + $(this).attr("trackid") +"'); $(this).parents('tr').remove();\">" +
                                    "<span class=\"glyphicon glyphicon-trash\"></span></a>")
                                .find('a').fadeTo('fast',1);
                            },
                            mouseleave: function(){
                                $(this).children().last().find("a").stop().remove();
                            }
                        });
                    };

                    $('#salamisandwich > tbody > tr').on({
                        click: function() {
                            $('#salamisandwich > tbody > tr').removeClass('active');
                            socket.send('MPD_API_PLAY_TRACK,'+$(this).attr('trackid'));
                            $(this).addClass('active');
                        },
                    });
                    break;
                case "search":
                    $('#wait').modal('hide');
                case "browse":
                    if(current_app !== 'browse' && current_app !== 'search')
                        break;

                    /* The use of encodeURI() below might seem useless, but it's not. It prevents
                     * some browsers, such as Safari, from changing the normalization form of the
                     * URI from NFD to NFC, breaking our link with MPD.
                     */
                    for (var item in obj.data) {
                        switch(obj.data[item].type) {
                            case "directory":
                                $('#salamisandwich > tbody').append(
                                    "<tr uri=\"" + encodeURI(obj.data[item].dir) + "\" class=\"dir\">" +
                                    "<td><span class=\"glyphicon glyphicon-folder-open\"></span></td>" +
                                    "<td><a>" + basename(obj.data[item].dir) + "</a></td>" +
                                    "<td></td><td></td></tr>"
                                );
                                break;
                            case "playlist":
                                $('#salamisandwich > tbody').append(
                                    "<tr uri=\"" + encodeURI(obj.data[item].plist) + "\" class=\"plist\">" +
                                    "<td><span class=\"glyphicon glyphicon-list\"></span></td>" +
                                    "<td><a>" + basename(obj.data[item].plist) + "</a></td>" +
                                    "<td></td><td></td></tr>"
                                );
                                break;
                            case "song":
                                var minutes = Math.floor(obj.data[item].duration / 60);
                                var seconds = obj.data[item].duration - minutes * 60;

                                $('#salamisandwich > tbody').append(
                                    "<tr uri=\"" + encodeURI(obj.data[item].uri) + "\" class=\"song\">" +
                                    "<td><span class=\"glyphicon glyphicon-music\"></span></td>" +
                                    "<td>" + obj.data[item].title +"</td>" +
                                    "<td>"+ minutes + ":" + (seconds < 10 ? '0' : '') + seconds +
                                    "</td><td></td></tr>"
                                );
                                break;
                            case "wrap":
                                if(current_app == 'browse') {
                                    $('#next').removeClass('hide');
                                } else {
                                    $('#salamisandwich > tbody').append(
                                        "<tr><td><span class=\"glyphicon glyphicon-remove\"></span></td>" + 
                                        "<td>Too many results, please refine your search!</td>" + 
                                        "<td></td><td></td></tr>"
                                    );
                                }
                                break;
                        }

                        if(pagination > 0)
                            $('#prev').removeClass('hide');

                    }

                    function appendClickableIcon(appendTo, onClickAction, glyphicon) {
                        $(appendTo).append(
                            "<a role=\"button\" class=\"pull-right btn-group-hover\">" +
                            "<span class=\"glyphicon glyphicon-" + glyphicon + "\"></span></a>")
                            .find('a').click(function(e) {
                                e.stopPropagation();
                                socket.send(onClickAction + "," + decodeURI($(this).parents("tr").attr("uri")));
                            $('.top-right').notify({
                                message:{
                                    text: $('td:nth-child(2)', $(this).parents("tr")).text() + " added"
                                } }).show();
                            }).fadeTo('fast',1);
                    }

                    if ( isTouch ) {
                        appendClickableIcon($("#salamisandwich > tbody > tr.dir > td:last-child"), 'MPD_API_ADD_TRACK', 'plus');
                        appendClickableIcon($("#salamisandwich > tbody > tr.song > td:last-child"), 'MPD_API_ADD_TRACK', 'play');
                    } else {
                        $('#salamisandwich > tbody > tr').on({
                            mouseenter: function() {
                                if($(this).is(".dir")) 
                                    appendClickableIcon($(this).children().last(), 'MPD_API_ADD_TRACK', 'plus');
                                else if($(this).is(".song"))
                                    appendClickableIcon($(this).children().last(), 'MPD_API_ADD_PLAY_TRACK', 'play');
                            },
                            mouseleave: function(){
                                $(this).children().last().find("a").stop().remove();
                            }
                        });
                    };
                    $('#salamisandwich > tbody > tr').on({
                        click: function() {
                            switch($(this).attr('class')) {
                                case 'dir':
                                    pagination = 0;
                                    browsepath = $(this).attr("uri");
                                    $("#browse > a").attr("href", '#/browse/'+pagination+'/'+browsepath);
                                    app.setLocation('#/browse/'+pagination+'/'+browsepath);
                                    break;
                                case 'song':
                                    socket.send("MPD_API_ADD_TRACK," + decodeURI($(this).attr("uri")));
                                    $('.top-right').notify({
                                        message:{
                                            text: $('td:nth-child(2)', this).text() + " added"
                                        }
                                    }).show();
                                    break;
                                case 'plist':
                                    socket.send("MPD_API_ADD_PLAYLIST," + decodeURI($(this).attr("uri")));
                                    $('.top-right').notify({
                                        message:{
                                            text: "Playlist " + $('td:nth-child(2)', this).text() + " added"
                                        }
                                    }).show();
                                    break;
                            }
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

                    if(obj.data.crossfade)
                        $('#btncrossfade').addClass("active")
                    else
                        $('#btncrossfade').removeClass("active");

                    if(obj.data.repeat)
                        $('#btnrepeat').addClass("active")
                    else
                        $('#btnrepeat').removeClass("active");

                    last_state = obj;
                    break;
                case "outputnames":
                    $('#btn-outputs-block button').remove();
                    $.each(obj.data, function(id, name){
                        var btn = $('<button id="btnoutput'+id+'" class="btn btn-default" onclick="toggleoutput(this, '+id+')"><span class="glyphicon glyphicon-volume-up"></span> '+name+'</button>');
                        btn.appendTo($('#btn-outputs-block'));
                    });
                    /* remove cache, since the buttons have been recreated */
                    last_outputs = '';
                    break;
                case "outputs":
                    if(JSON.stringify(obj) === JSON.stringify(last_outputs))
                        break;
                    $.each(obj.data, function(id, enabled){
                        if (enabled)
                        $('#btnoutput'+id).addClass("active");
                        else
                        $('#btnoutput'+id).removeClass("active");
                    });
                    last_outputs = obj;
                    break;
                case "disconnected":
                    if($('.top-right').has('div').length == 0)
                        $('.top-right').notify({
                            message:{text:"ympd lost connection to MPD "},
                            type: "danger",
                            fadeOut: { enabled: true, delay: 1000 },
                        }).show();
                    break;
                case "update_queue":
                    if(current_app === 'queue')
                        socket.send('MPD_API_GET_QUEUE,'+pagination);
                    break;
                case "song_change":

                    $('#album').text("");
                    $('#artist').text("");

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

                    if ($.cookie("notification") === "true")
                        songNotify(obj.data.title, obj.data.artist, obj.data.album );
                    else
                        $('.top-right').notify({
                            message:{html: notification},
                            type: "info",
                        }).show();
                        
                    break;
                case "mpdhost":
                    $('#mpdhost').val(obj.data.host);
                    $('#mpdport').val(obj.data.port);
                    if(obj.data.passwort_set)
                        $('#mpd_password_set').removeClass('hide');
                    break;
                case "error":
                    $('.top-right').notify({
                        message:{text: obj.data},
                        type: "danger",
                    }).show();
                    break;
                case "scheduleList":
                    $('#mpdScheduler_panel > table > tbody').empty();
                    obj.data.forEach(function(job) {
                        $('#mpdScheduler_panel > table > tbody').append(
                            "<tr data-uuid=\"" + job.uuid + "\">" + 
                                "<td>" + job.job + "</td>" +
                                "<td>"+ job.time +"</td>" + 
                                "<td><a class=\"pull-right btn-group-hover\" href=\"#/\" " +
                                        "onclick=\"socket.send('MPD_API_SCHEDULE_CANCEL,' + $(this).parents('tr').data('uuid')); $(this).parents('tr').remove();return false;\">" +
                                "<span class=\"glyphicon glyphicon-trash\"></span></a></td>" +
                            "</tr>"
                        );
                    });
                    break;
                default:
                    break;
            }


        }
        socket.onclose = function(){
            console.log("disconnected");
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

    u = u.split('#');

    return pcol + u[0] + "/ws";
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
$('#btncrossfade').on('click', function(e) {
    socket.send("MPD_API_TOGGLE_CROSSFADE," + ($(this).hasClass('active') ? 0 : 1));
});
$('#btnrepeat').on('click', function (e) {
    socket.send("MPD_API_TOGGLE_REPEAT," + ($(this).hasClass('active') ? 0 : 1));
});

function toggleoutput(button, id) {
    socket.send("MPD_API_TOGGLE_OUTPUT,"+id+"," + ($(button).hasClass('active') ? 0 : 1));
}

$('#btnnotify').on('click', function (e) {
    if($.cookie("notification") === "true") {
        $.cookie("notification", false);
    } else {
        Notification.requestPermission(function (permission) {
            if(!('permission' in Notification)) {
                Notification.permission = permission;
            }

            if (permission === "granted") {
                $.cookie("notification", true, { expires: 424242 });
                $('btnnotify').addClass("active");
            }
        });
    }
});

function getHost() {
    socket.send('MPD_API_GET_MPDHOST');

    function onEnter(event) {
      if ( event.which == 13 ) {
        confirmSettings();
      }
    }

    $('#mpdhost').keypress(onEnter);
    $('#mpdport').keypress(onEnter);
    $('#mpd_pw').keypress(onEnter);
    $('#mpd_pw_con').keypress(onEnter);
}

$('#search').submit(function () {
    app.setLocation("#/search/"+$('#search > div > input').val());
    $('#wait').modal('show');
    setTimeout(function() {
        $('#wait').modal('hide');
    }, 10000);
    return false;
});

$('.page-btn').on('click', function (e) {

    switch ($(this).text()) {
        case "Next":
            if (current_app == "dirble") dirble_page++;
            else pagination += MAX_ELEMENTS_PER_PAGE;
            break;
        case "Previous":
            if (current_app == "dirble") dirble_page--
            else {
                pagination -= MAX_ELEMENTS_PER_PAGE;
                if(pagination <= 0)
                    pagination = 0;
            }
            break;
    }

    switch(current_app) {
        case "queue":
            app.setLocation('#/'+pagination);
            break;
        case "browse":
            app.setLocation('#/browse/'+pagination+'/'+browsepath);
            break;
        case "dirble":
            app.setLocation("#/dirble/"+dirble_catid+"/"+dirble_page);
            break;
    }
    e.preventDefault();
});

function addStream() {
    if($('#streamurl').val().length > 0) {
    	socket.send('MPD_API_ADD_TRACK,'+$('#streamurl').val());
    }
    $('#streamurl').val("");
    $('#addstream').modal('hide');
}

function saveQueue() {
    if($('#playlistname').val().length > 0) {
    	socket.send('MPD_API_SAVE_QUEUE,'+$('#playlistname').val());
    }
    $('#savequeue').modal('hide');
}

function confirmSettings() {
    if($('#mpd_pw').val().length + $('#mpd_pw_con').val().length > 0) {
        if ($('#mpd_pw').val() !== $('#mpd_pw_con').val())
        {
            $('#mpd_pw_con').popover('show');
            setTimeout(function() {
                $('#mpd_pw_con').popover('hide');
            }, 2000);
            return;
        } else
            socket.send('MPD_API_SET_MPDPASS,'+$('#mpd_pw').val());
    }
    socket.send('MPD_API_SET_MPDHOST,'+$('#mpdport').val()+','+$('#mpdhost').val());
    $('#settings').modal('hide');
}

$('#mpd_password_set > button').on('click', function (e) {
    socket.send('MPD_API_SET_MPDPASS,');
    $('#mpd_pw').val("");
    $('#mpd_pw_con').val("");
    $('#mpd_password_set').addClass('hide');
})

function notificationsSupported() {
    return "Notification" in window;
}

function songNotify(title, artist, album) {
    /*var opt = {
        type: "list",
        title: title,
        message: title,
        items: []
    }
    if(artist.length > 0)
        opt.items.push({title: "Artist", message: artist});
    if(album.length > 0)
        opt.items.push({title: "Album", message: album});
*/
    //chrome.notifications.create(id, options, creationCallback);

    var textNotification = "";
    if(typeof artist != 'undefined' && artist.length > 0)
        textNotification += " " + artist;
    if(typeof album != 'undefined' && album.length > 0)
        textNotification += "\n " + album;

	var notification = new Notification(title, {icon: 'assets/favicon.ico', body: textNotification});
    setTimeout(function(notification) {
        notification.close();
    }, 3000, notification);
}

$(document).keydown(function(e){
    if (e.target.tagName == 'INPUT') {
        return;
    }
    switch (e.which) {
        case 37: //left
            socket.send('MPD_API_SET_PREV');
            break;
        case 39: //right
            socket.send('MPD_API_SET_NEXT');
            break;
        case 32: //space
            clickPlay();
            break;
        default:
            return;
    }
    e.preventDefault();
});

function dirble_load_categories() {

    dirble_page = 1;

    $.getJSON( "http://api.dirble.com/v2/categories?token=2e223c9909593b94fc6577361a", function( data ) {

        $('#dirble_loading').addClass('hide');

        data = data.sort(function(a, b) {
            return (a.title > b.title) ? 1 : 0;
        });

        var max = data.length - data.length%2;

        for(i = 0; i < max; i+=2) {

            $('#dirble_left > tbody').append(
                "<tr><td catid=\""+data[i].id+"\">"+data[i].title+"</td></tr>"
            );

            $('#dirble_right > tbody').append(
                "<tr><td catid=\""+data[i+1].id+"\">"+data[i+1].title+"</td></tr>"
            );
        }

        if (max != data.length) {
            $('#dirble_left > tbody').append(
                "<tr><td catid=\""+data[max].id+"\">"+data[max].title+"</td></tr>"
            );
        }

        $('#dirble_left > tbody > tr > td').on({
            click: function() {
                dirble_selected_cat = $(this).text();
                dirble_catid = $(this).attr("catid");
                app.setLocation("#/dirble/"+dirble_catid+"/"+dirble_page);
            }
        });

        $('#dirble_right > tbody > tr > td').on({
            click: function() {
                dirble_selected_cat = $(this).text();
                dirble_catid = $(this).attr("catid");
                app.setLocation("#/dirble/"+dirble_catid+"/"+dirble_page);
            }
        });
    });
}


function dirble_load_stations() {

    $.getJSON( "http://api.dirble.com/v2/category/"+dirble_catid+"/stations?page="+dirble_page+"&per_page=20&token=2e223c9909593b94fc6577361a", function( data ) {

        $('#dirble_loading').addClass('hide');
        if (data.length == 20) $('#next').removeClass('hide');

        var max = data.length - data.length%2;

        for(i = 0; i < max; i+=2) {

            $('#dirble_left > tbody').append(
                "<tr><td radioid=\""+data[i].id+"\">"+data[i].name+"</td></tr>"
            );
            $('#dirble_right > tbody').append(
                "<tr><td radioid=\""+data[i+1].id+"\">"+data[i+1].name+"</td></tr>"
            );
        }

        if (max != data.length) {
            $('#dirble_left > tbody').append(
                "<tr><td radioid=\""+data[max].id+"\">"+data[max].name+"</td></tr>"
            );
        }

        $('#dirble_left > tbody > tr > td').on({
            click: function() {
                var _this = $(this);

                $.getJSON( "http://api.dirble.com/v2/station/"+$(this).attr("radioid")+"?token=2e223c9909593b94fc6577361a", function( data ) {

                    socket.send("MPD_API_ADD_TRACK," + data.streams[0].stream);
                    $('.top-right').notify({
                        message:{
                            text: _this.text() + " added"
                        }
                    }).show();
                });
            },
            mouseenter: function() {
                var _this = $(this);

                $(this).last().append(
                "<a role=\"button\" class=\"pull-right btn-group-hover\">" +
                "<span class=\"glyphicon glyphicon-play\"></span></a>").find('a').click(function(e) {
                    e.stopPropagation();

                    $.getJSON( "http://api.dirble.com/v2/station/"+_this.attr("radioid")+"?token=2e223c9909593b94fc6577361a", function( data ) {

                        socket.send("MPD_API_ADD_PLAY_TRACK," + data.streams[0].stream);
                        $('.top-right').notify({
                            message:{
                                text: _this.text() + " added"
                            }
                        }).show();
                    });
                }).fadeTo('fast',1);
            },

            mouseleave: function(){
                $(this).last().find("a").stop().remove();
            }
        });

        $('#dirble_right> tbody > tr > td').on({
            click: function() {
                var _this = $(this);

                $.getJSON( "http://api.dirble.com/v2/station/"+$(this).attr("radioid")+"?token=2e223c9909593b94fc6577361a", function( data ) {

                    socket.send("MPD_API_ADD_TRACK," + data.streams[0].stream);
                    $('.top-right').notify({
                        message:{
                            text: _this.text() + " added"
                        }
                    }).show();
                });
            },
            mouseenter: function() {
                var _this = $(this);

                $(this).last().append(
                "<a role=\"button\" class=\"pull-right btn-group-hover\">" +
                "<span class=\"glyphicon glyphicon-play\"></span></a>").find('a').click(function(e) {
                    e.stopPropagation();

                    $.getJSON( "http://api.dirble.com/v2/station/"+_this.attr("radioid")+"?token=2e223c9909593b94fc6577361a", function( data ) {

                        socket.send("MPD_API_ADD_PLAY_TRACK," + data.streams[0].stream);
                        $('.top-right').notify({
                            message:{
                                text: _this.text() + " added"
                            }
                        }).show();
                    });
                }).fadeTo('fast',1);
            },

            mouseleave: function(){
                $(this).last().find("a").stop().remove();
            }
        });
    });
}
