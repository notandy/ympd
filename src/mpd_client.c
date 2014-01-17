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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/song.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>

#include "mpd_client.h"

struct mpd_connection *conn = NULL;
enum mpd_conn_states mpd_conn_state = MPD_DISCONNECTED;
enum mpd_state mpd_play_state = MPD_STATE_UNKNOWN;

int callback_ympd(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason,
        void *user, void *in, size_t len)
{
    size_t n,  m = -1;
    char *buf = NULL, *p;
    struct per_session_data__ympd *pss = (struct per_session_data__ympd *)user;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            lwsl_info("mpd_client: "
                    "LWS_CALLBACK_ESTABLISHED\n");
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            buf = (char *)malloc(MAX_SIZE + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);
            if(buf == NULL) {
                lwsl_err("ERROR Failed allocating memory\n");
                return -1;
            }
            p = &buf[LWS_SEND_BUFFER_PRE_PADDING];

            if(pss->do_send & DO_SEND_ERROR) {
                n = snprintf(p, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
                    mpd_connection_get_error_message(conn));
                pss->do_send &= ~DO_SEND_ERROR;

                /* Try to recover error */
                if (!mpd_connection_clear_error(conn))
                    mpd_conn_state = MPD_FAILURE;
            }
            else if(mpd_conn_state != MPD_CONNECTED) {
                n = snprintf(p, MAX_SIZE, "{\"type\":\"disconnected\"}");
            }
            else if(pss->do_send & DO_SEND_PLAYLIST) {
                /*n = mpd_put_playlist(p);*/
                n = snprintf(p, MAX_SIZE, "{\"type\":\"update_playlist\"}");
                pss->do_send &= ~DO_SEND_PLAYLIST;
            }
            else if(pss->do_send & DO_SEND_TRACK_INFO) {
                n = mpd_put_current_song(p);
                pss->do_send &= ~DO_SEND_TRACK_INFO;
            }
            else if(pss->do_send & DO_SEND_BROWSE) {
                n = mpd_put_browse(p, pss->browse_path);
                pss->do_send &= ~DO_SEND_BROWSE;
                free(pss->browse_path);
            }
            else {
                /* Default Action */
                int current_song_id;
                unsigned queue_version;

                n = mpd_put_state(p, &current_song_id, &queue_version);
                if(current_song_id != pss->current_song_id)
                {
                    pss->current_song_id = current_song_id;
                    pss->do_send |= DO_SEND_TRACK_INFO;
                    libwebsocket_callback_on_writable(context, wsi);
                }
                else if(pss->queue_version != queue_version) {
                    pss->queue_version = queue_version;
                    pss->do_send |= DO_SEND_PLAYLIST;
                    libwebsocket_callback_on_writable(context, wsi);
                }
            }

            if(n > 0)
                m = libwebsocket_write(wsi, (unsigned char *)p, n, LWS_WRITE_TEXT);

            if (m < n) {
                lwsl_err("ERROR %d writing to socket\n", n, m);
                free(buf);
                return -1;
            }
            free(buf);
            break;

        case LWS_CALLBACK_RECEIVE:
            if(!strcmp((const char *)in, MPD_API_GET_PLAYLIST))
                pss->do_send |= DO_SEND_PLAYLIST;
            else if(!strcmp((const char *)in, MPD_API_GET_TRACK_INFO))
                pss->do_send |= DO_SEND_TRACK_INFO;
            else if(!strcmp((const char *)in, MPD_API_UPDATE_DB))
                mpd_run_update(conn, NULL);
            else if(!strcmp((const char *)in, MPD_API_SET_PAUSE))
                mpd_run_toggle_pause(conn);
            else if(!strcmp((const char *)in, MPD_API_SET_PREV))
                mpd_run_previous(conn);
            else if(!strcmp((const char *)in, MPD_API_SET_NEXT))
                mpd_run_next(conn);
            else if(!strcmp((const char *)in, MPD_API_SET_PLAY))
                mpd_run_play(conn);
            else if(!strcmp((const char *)in, MPD_API_SET_STOP))
                mpd_run_stop(conn);
            else if(!strcmp((const char *)in, MPD_API_RM_ALL))
                mpd_run_clear(conn);
            else if(!strncmp((const char *)in, MPD_API_RM_TRACK, sizeof(MPD_API_RM_TRACK)-1)) {
                unsigned id;
                if(sscanf(in, "MPD_API_RM_TRACK,%u", &id))
                    mpd_run_delete_id(conn, id);

                libwebsocket_callback_on_writable(context, wsi);
            }
            else if(!strncmp((const char *)in, MPD_API_PLAY_TRACK, sizeof(MPD_API_PLAY_TRACK)-1)) {
                unsigned id;
                if(sscanf(in, "MPD_API_PLAY_TRACK,%u", &id))
                    mpd_run_play_id(conn, id);
            }
            else if(!strncmp((const char *)in, MPD_API_TOGGLE_RANDOM, sizeof(MPD_API_TOGGLE_RANDOM)-1)) {
                unsigned random;
                if(sscanf(in, "MPD_API_TOGGLE_RANDOM,%u", &random))
                    mpd_run_random(conn, random);
            }
            else if(!strncmp((const char *)in, MPD_API_TOGGLE_REPEAT, sizeof(MPD_API_TOGGLE_REPEAT)-1)) {
                unsigned repeat;
                if(sscanf(in, "MPD_API_TOGGLE_REPEAT,%u", &repeat))
                    mpd_run_repeat(conn, repeat);
            }
            else if(!strncmp((const char *)in, MPD_API_TOGGLE_CONSUME, sizeof(MPD_API_TOGGLE_CONSUME)-1)) {
                unsigned consume;
                if(sscanf(in, "MPD_API_TOGGLE_CONSUME,%u", &consume))
                    mpd_run_consume(conn, consume);
            }
            else if(!strncmp((const char *)in, MPD_API_TOGGLE_SINGLE, sizeof(MPD_API_TOGGLE_SINGLE)-1)) {
                unsigned single;
                if(sscanf(in, "MPD_API_TOGGLE_SINGLE,%u", &single))
                    mpd_run_single(conn, single);
            }
            else if(!strncmp((const char *)in, MPD_API_SET_VOLUME, sizeof(MPD_API_SET_VOLUME)-1)) {
                unsigned int volume;
                if(sscanf(in, "MPD_API_SET_VOLUME,%ud", &volume) && volume <= 100)
                    mpd_run_set_volume(conn, volume);
            }
            else if(!strncmp((const char *)in, MPD_API_SET_SEEK, sizeof(MPD_API_SET_SEEK)-1)) {
                unsigned int seek, songid;
                if(sscanf(in, "MPD_API_SET_SEEK,%u,%u", &songid, &seek)) {
                    mpd_run_seek_id(conn, songid, seek);
                }
            }
            else if(!strncmp((const char *)in, MPD_API_GET_BROWSE, sizeof(MPD_API_GET_BROWSE)-1)) {
                char *dir;
                if(sscanf(in, "MPD_API_GET_BROWSE,%m[^\t\n]", &dir) && dir != NULL) {
                    pss->do_send |= DO_SEND_BROWSE;
                    pss->browse_path = dir;
                }
            }
            else if(!strncmp((const char *)in, MPD_API_ADD_TRACK, sizeof(MPD_API_ADD_TRACK)-1)) {
                char *uri;
                if(sscanf(in, "MPD_API_ADD_TRACK,%m[^\t\n]", &uri) && uri != NULL) {
                    mpd_run_add(conn, uri);
                    free(uri);
                }
            }
            else if(!strncmp((const char *)in, MPD_API_ADD_PLAY_TRACK, sizeof(MPD_API_ADD_PLAY_TRACK)-1)) {
                char *uri;
                if(sscanf(in, "MPD_API_ADD_PLAY_TRACK,%m[^\t\n]", &uri) && uri != NULL) {
                    int added_song = mpd_run_add_id(conn, uri);
                    if(added_song != -1)
                        mpd_run_play_id(conn, added_song);
                    free(uri);
                }
            }
            

            if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
                pss->do_send |= DO_SEND_ERROR;

            break;

        default:
            break;
    }

    return 0;
}

void mpd_loop()
{
    switch (mpd_conn_state) {
        case MPD_DISCONNECTED:
            /* Try to connect */
            lwsl_notice("MPD Connecting to %s:%d\n", mpd_host, mpd_port);
            conn = mpd_connection_new(mpd_host, mpd_port, 3000);
            if (conn == NULL) {
                lwsl_err("Out of memory.");
                mpd_conn_state = MPD_FAILURE;
                return;
            }

            if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
                lwsl_err("MPD connection: %s\n", mpd_connection_get_error_message(conn));
                mpd_conn_state = MPD_FAILURE;
                return;
            }

            lwsl_notice("MPD connected.\n");
            mpd_conn_state = MPD_CONNECTED;
            break;

        case MPD_FAILURE:
            lwsl_err("MPD connection failed.\n");

            if(conn != NULL)
                mpd_connection_free(conn);
            conn = NULL;
            mpd_conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            /* Nothing to do */
            break;
    }
}

char* mpd_get_title(struct mpd_song const *song)
{
    char *str, *ptr;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if(str == NULL)
        str = (char *)mpd_song_get_uri(song);

    if(str == NULL)
        return NULL;

    ptr = str;
    while(*ptr++ != '\0')
        if(*ptr=='"')
            *ptr='\'';

    return basename(str);
}

int mpd_put_state(char *buffer, int *current_song_id, unsigned *queue_version)
{
    struct mpd_status *status;
    int len;

    status = mpd_run_status(conn);
    if (!status) {
        lwsl_err("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(conn));
        mpd_conn_state = MPD_FAILURE;
        return 0;
    }

    len = snprintf(buffer, MAX_SIZE,
            "{\"type\":\"state\", \"data\":{"
            " \"state\":%d, \"volume\":%d, \"repeat\":%d,"
            " \"single\":%d, \"consume\":%d, \"random\":%d, "
            " \"songpos\": %d, \"elapsedTime\": %d, \"totalTime\":%d, "
            " \"currentsongid\": %d"
            "}}", 
            mpd_status_get_state(status),
            mpd_status_get_volume(status), 
            mpd_status_get_repeat(status),
            mpd_status_get_single(status),
            mpd_status_get_consume(status),
            mpd_status_get_random(status),
            mpd_status_get_song_pos(status),
            mpd_status_get_elapsed_time(status),
            mpd_status_get_total_time(status),
            mpd_status_get_song_id(status));

    *current_song_id = mpd_status_get_song_id(status);
    *queue_version = mpd_status_get_queue_version(status);
    mpd_status_free(status);
    return len;
}

int mpd_put_current_song(char *buffer)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;

    song = mpd_run_current_song(conn);
    if(song == NULL)
        return 0;

    cur += snprintf(cur, end - cur, "{\"type\": \"song_change\", \"data\":"
            "{\"pos\":%d, \"title\":\"%s\"",
            mpd_song_get_pos(song),
            mpd_get_title(song));
    if(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) != NULL)
        cur += snprintf(cur, end - cur, ", \"artist\":\"%s\"",
            mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
    if(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
        cur += snprintf(cur, end - cur, ", \"album\":\"%s\"",
            mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));

    cur += snprintf(cur, end - cur, "}}");
    mpd_song_free(song);
    mpd_response_finish(conn);

    return cur - buffer;
}

int mpd_put_playlist(char *buffer)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;

    if (!mpd_send_list_queue_meta(conn)) {
        lwsl_err("MPD mpd_send_list_queue_meta: %s\n", mpd_connection_get_error_message(conn));
        cur += snprintf(cur, end  - cur, "{\"type\":\"error\",\"data\":\"%s\"}", 
            mpd_connection_get_error_message(conn));

        if (!mpd_connection_clear_error(conn))
            mpd_conn_state = MPD_FAILURE;
        return cur - buffer;
    }

    cur += snprintf(cur, end  - cur, "{\"type\": \"playlist\", \"data\": [ ");

    while((entity = mpd_recv_entity(conn)) != NULL) {
        const struct mpd_song *song;

        if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            cur += snprintf(cur, end  - cur, 
                    "{\"id\":%d, \"pos\":%d, \"duration\":%d, \"title\":\"%s\"},",
                    mpd_song_get_id(song),
                    mpd_song_get_pos(song),
                    mpd_song_get_duration(song),
                    mpd_get_title(song)
                    );
        }
        mpd_entity_free(entity);
    }

    /* remove last ',' */
    cur--;
    cur += snprintf(cur, end  - cur, "] }");
    return cur - buffer;
}

int mpd_put_browse(char *buffer, char *path)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;

    if (!mpd_send_list_meta(conn, path)) {
        lwsl_err("MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(conn));
        cur += snprintf(cur, end  - cur, "{\"type\":\"error\",\"data\":\"%s\"}", 
            mpd_connection_get_error_message(conn));

        if (!mpd_connection_clear_error(conn))
            mpd_conn_state = MPD_FAILURE;
        return cur - buffer;
    }
    cur += snprintf(cur, end  - cur, "{\"type\":\"browse\",\"data\":[ ");

    while((entity = mpd_recv_entity(conn)) != NULL) {
        const struct mpd_song *song;
        const struct mpd_directory *dir;
        const struct mpd_playlist *pl;

        switch (mpd_entity_get_type(entity)) {
            case MPD_ENTITY_TYPE_UNKNOWN:
                break;

            case MPD_ENTITY_TYPE_SONG:
                song = mpd_entity_get_song(entity);
                cur += snprintf(cur, end  - cur, 
                        "{\"type\":\"song\",\"uri\":\"%s\",\"duration\":%d,\"title\":\"%s\"},",
                        mpd_song_get_uri(song),
                        mpd_song_get_duration(song),
                        mpd_get_title(song)
                        );
                break;

            case MPD_ENTITY_TYPE_DIRECTORY:
                dir = mpd_entity_get_directory(entity);
                cur += snprintf(cur, end  - cur, 
                        "{\"type\":\"directory\",\"dir\":\"%s\", \"basename\":\"%s\"},",
                        mpd_directory_get_path(dir), 
                        basename((char *)mpd_directory_get_path(dir))
                        );
                break;

            case MPD_ENTITY_TYPE_PLAYLIST:
                pl = mpd_entity_get_playlist(entity);
                cur += snprintf(cur, end  - cur, 
                        "{\"type\":\"playlist\",\"plist\":\"%s\"},",
                        mpd_playlist_get_path(pl)
                        );
                break;
        }
        mpd_entity_free(entity);
    }

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(conn)) {
        lwsl_err("MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(conn));
        mpd_conn_state = MPD_FAILURE;
        return 0;
    }

    /* remove last ',' */
    cur--;
    cur += snprintf(cur, end  - cur, "] }");
    return cur - buffer;
}
