/* ympd
   (c) 2013-2014 Andrew Karpow <andy@ympd.org>
   This project's homepage is: http://www.ympd.org

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
   
#ifndef __MPD_CLIENT_H__
#define __MPD_CLIENT_H__

#include <libwebsockets.h>

#define MAX_SIZE 1024 * 100

#define DO_SEND_STATE      (1 << 0)
#define DO_SEND_PLAYLIST   (1 << 1)
#define DO_SEND_TRACK_INFO (1 << 2)
#define DO_SEND_BROWSE     (1 << 3)
#define DO_SEND_ERROR      (1 << 4)


#define MPD_API_GET_SEEK         "MPD_API_GET_SEEK"
#define MPD_API_GET_PLAYLIST     "MPD_API_GET_PLAYLIST"
#define MPD_API_GET_TRACK_INFO   "MPD_API_GET_TRACK_INFO"
#define MPD_API_GET_BROWSE       "MPD_API_GET_BROWSE"
#define MPD_API_ADD_TRACK        "MPD_API_ADD_TRACK"
#define MPD_API_ADD_PLAY_TRACK   "MPD_API_ADD_PLAY_TRACK"
#define MPD_API_PLAY_TRACK       "MPD_API_PLAY_TRACK"
#define MPD_API_RM_TRACK         "MPD_API_RM_TRACK"
#define MPD_API_RM_ALL           "MPD_API_RM_ALL"
#define MPD_API_SET_VOLUME       "MPD_API_SET_VOLUME"
#define MPD_API_SET_PAUSE        "MPD_API_SET_PAUSE"
#define MPD_API_SET_PLAY         "MPD_API_SET_PLAY"
#define MPD_API_SET_STOP         "MPD_API_SET_STOP"
#define MPD_API_SET_SEEK         "MPD_API_SET_SEEK"
#define MPD_API_SET_NEXT         "MPD_API_SET_PREV"
#define MPD_API_SET_PREV         "MPD_API_SET_NEXT"
#define MPD_API_UPDATE_DB        "MPD_API_UPDATE_DB"
#define MPD_API_TOGGLE_RANDOM    "MPD_API_TOGGLE_RANDOM"
#define MPD_API_TOGGLE_CONSUME   "MPD_API_TOGGLE_CONSUME"
#define MPD_API_TOGGLE_SINGLE    "MPD_API_TOGGLE_SINGLE"
#define MPD_API_TOGGLE_REPEAT    "MPD_API_TOGGLE_REPEAT"

struct per_session_data__ympd {
    int do_send;
    unsigned queue_version;
    int current_song_id;
    char *browse_path;
};

enum mpd_conn_states {
    MPD_FAILURE,
    MPD_DISCONNECTED,
    MPD_CONNECTED
};

void *mpd_idle_connection(void *_data);
int callback_ympd(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason,
        void *user, void *in, size_t len);
void mpd_loop();
int mpd_put_state(char *buffer, int *current_song_id, unsigned *queue_version);
int mpd_put_current_song(char *buffer);
int mpd_put_playlist(char *buffer);
int mpd_put_browse(char *buffer, char *path);

int mpd_port;
const char *mpd_host;

#endif

