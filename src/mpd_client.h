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

#include "mongoose.h"

#define MAX_SIZE 1024 * 100
#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,
#define MPD_CMDS(X) \
    X(MPD_API_GET_PLAYLIST) \
    X(MPD_API_GET_BROWSE) \
    X(MPD_API_GET_MPDHOST) \
    X(MPD_API_ADD_TRACK) \
    X(MPD_API_ADD_PLAY_TRACK) \
    X(MPD_API_PLAY_TRACK) \
    X(MPD_API_RM_TRACK) \
    X(MPD_API_RM_ALL) \
    X(MPD_API_SET_VOLUME) \
    X(MPD_API_SET_PAUSE) \
    X(MPD_API_SET_PLAY) \
    X(MPD_API_SET_STOP) \
    X(MPD_API_SET_SEEK) \
    X(MPD_API_SET_NEXT) \
    X(MPD_API_SET_PREV) \
    X(MPD_API_SET_MPDHOST) \
    X(MPD_API_SET_MPDPASS) \
    X(MPD_API_UPDATE_DB) \
    X(MPD_API_TOGGLE_RANDOM) \
    X(MPD_API_TOGGLE_CONSUME) \
    X(MPD_API_TOGGLE_SINGLE) \
    X(MPD_API_TOGGLE_REPEAT)

enum mpd_cmd_ids {
    MPD_CMDS(GEN_ENUM)
};

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT
};

struct t_mpd {
    int port;
    char host[128];
    char *password;

    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;

    /* Reponse Buffer */
    char buf[MAX_SIZE];
    size_t buf_size;

    int song_id;
    unsigned queue_version;
} mpd;

struct t_mpd_client_session {
    int song_id;
    unsigned queue_version;
};

void mpd_poll(struct mg_server *s);
int callback_mpd(struct mg_connection *c);
int mpd_close_handler(struct mg_connection *c);
int mpd_put_state(char *buffer, int *current_song_id, unsigned *queue_version);
int mpd_put_current_song(char *buffer);
int mpd_put_playlist(char *buffer);
int mpd_put_browse(char *buffer, char *path);
void mpd_disconnect();
#endif

