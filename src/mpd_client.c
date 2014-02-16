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

#include "mpd_client.h"
#include "config.h"

const char * mpd_cmd_strs[] = {
    MPD_CMDS(GEN_STR)
};

static inline enum mpd_cmd_ids get_cmd_id(char *cmd)
{
    for(int i = 0; i < sizeof(mpd_cmd_strs)/sizeof(mpd_cmd_strs[0]); i++)
        if(!strncmp(cmd, mpd_cmd_strs[i], strlen(mpd_cmd_strs[i])))
            return i;

    return -1;
}

int callback_mpd(struct mg_connection *c)
{
    enum mpd_cmd_ids cmd_id = get_cmd_id(c->content);
    size_t n = 0;
    unsigned int uint_buf, uint_buf_2;
    int int_buf;
    char *p_charbuf;

    if(cmd_id == -1)
        return MG_CLIENT_CONTINUE;

    if(mpd.conn_state != MPD_CONNECTED && cmd_id != MPD_API_SET_MPDHOST &&
        cmd_id != MPD_API_GET_MPDHOST && cmd_id != MPD_API_SET_MPDPASS)
        return MG_CLIENT_CONTINUE;

    switch(cmd_id)
    {
        case MPD_API_UPDATE_DB:
            mpd_run_update(mpd.conn, NULL);
            break;
        case MPD_API_SET_PAUSE:
            mpd_run_toggle_pause(mpd.conn);
            break;
        case MPD_API_SET_PREV:
            mpd_run_previous(mpd.conn);
            break;
        case MPD_API_SET_NEXT:
            mpd_run_next(mpd.conn);
            break;
        case MPD_API_SET_PLAY:
            mpd_run_play(mpd.conn);
            break;
        case MPD_API_SET_STOP:
            mpd_run_stop(mpd.conn);
            break;
        case MPD_API_RM_ALL:
            mpd_run_clear(mpd.conn);
            break;
        case MPD_API_GET_PLAYLIST:
            n = mpd_put_playlist(mpd.buf);
            break;
        case MPD_API_RM_TRACK:
            if(sscanf(c->content, "MPD_API_RM_TRACK,%u", &uint_buf))
                mpd_run_delete_id(mpd.conn, uint_buf);
            break;
        case MPD_API_PLAY_TRACK:
            if(sscanf(c->content, "MPD_API_PLAY_TRACK,%u", &uint_buf))
                mpd_run_play_id(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_RANDOM:
            if(sscanf(c->content, "MPD_API_TOGGLE_RANDOM,%u", &uint_buf))
                mpd_run_random(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_REPEAT:
            if(sscanf(c->content, "MPD_API_TOGGLE_REPEAT,%u", &uint_buf))
                mpd_run_repeat(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_CONSUME:
            if(sscanf(c->content, "MPD_API_TOGGLE_CONSUME,%u", &uint_buf))
                mpd_run_consume(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_SINGLE:
            if(sscanf(c->content, "MPD_API_TOGGLE_SINGLE,%u", &uint_buf))
                mpd_run_single(mpd.conn, uint_buf);
            break;
        case MPD_API_SET_VOLUME:
            if(sscanf(c->content, "MPD_API_SET_VOLUME,%ud", &uint_buf) && uint_buf <= 100)
                mpd_run_set_volume(mpd.conn, uint_buf);
            break;
        case MPD_API_SET_SEEK:
            if(sscanf(c->content, "MPD_API_SET_SEEK,%u,%u", &uint_buf, &uint_buf_2))
                mpd_run_seek_id(mpd.conn, uint_buf, uint_buf_2);
            break;
        case MPD_API_GET_BROWSE:
            if(sscanf(c->content, "MPD_API_GET_BROWSE,%m[^\t\n]", &p_charbuf) && p_charbuf != NULL)
                n = mpd_put_browse(mpd.buf, p_charbuf);
            else
                n = mpd_put_browse(mpd.buf, "/");
            free(p_charbuf);
            break;
        case MPD_API_ADD_TRACK:
            if(sscanf(c->content, "MPD_API_ADD_TRACK,%m[^\t\n]", &p_charbuf) && p_charbuf != NULL)
            {
                mpd_run_add(mpd.conn, p_charbuf);
                free(p_charbuf);
            }
            break;
        case MPD_API_ADD_PLAY_TRACK:
            if(sscanf(c->content, "MPD_API_ADD_PLAY_TRACK,%m[^\t\n]", &p_charbuf) && p_charbuf != NULL)
            {
                int_buf = mpd_run_add_id(mpd.conn, p_charbuf);
                if(int_buf != -1)
                    mpd_run_play_id(mpd.conn, int_buf);
                free(p_charbuf);
            }
#ifdef WITH_MPD_HOST_CHANGE
        /* Commands allowed when disconnected from MPD server */
        case MPD_API_SET_MPDHOST:
            int_buf = 0;
            if(sscanf(c->content, "MPD_API_SET_MPDHOST,%d,%m[^\t\n ]", &int_buf, &p_charbuf) && 
                p_charbuf != NULL && int_buf > 0)
            {
                strncpy(mpd.host, p_charbuf, sizeof(mpd.host));
                free(p_charbuf);
                mpd.port = int_buf;
                mpd.conn_state = MPD_RECONNECT;
                return MG_CLIENT_CONTINUE;
            }
            break;
        case MPD_API_GET_MPDHOST:
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"mpdhost\", \"data\": "
                "{\"host\" : \"%s\", \"port\": \"%d\", \"passwort_set\": %s}"
                "}", mpd.host, mpd.port, mpd.password ? "true" : "false");
            printf("mpd_password is %p\n", mpd.password);
        case MPD_API_SET_MPDPASS:
            if(sscanf(c->content, "MPD_API_SET_MPDPASS,%m[^\t\n ]", &p_charbuf) && 
                p_charbuf != NULL)
            {
                if(mpd.password)
                    free(mpd.password);

                mpd.password = p_charbuf;
                mpd.conn_state = MPD_RECONNECT;
                printf("Got mpd pw %s\n", mpd.password);
                return MG_CLIENT_CONTINUE;
            }
            break;
#endif
    }

    if(mpd.conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS)
    {
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
    }

    if(n > 0)
        mg_websocket_write(c, 1, mpd.buf, n);

    return MG_CLIENT_CONTINUE;
}

int mpd_close_handler(struct mg_connection *c)
{
    /* Cleanup session data */
    if(c->connection_param)
        free(c->connection_param);
    return 0;
}

static int mpd_notify_callback(struct mg_connection *c) {
    size_t n;

    if(!c->is_websocket)
        return MG_REQUEST_PROCESSED;

    if(c->callback_param)
    {
        /* error message? */
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\",\"data\":\"%s\"}", 
            (const char *)c->callback_param);

        mg_websocket_write(c, 1, mpd.buf, n);
        return MG_REQUEST_PROCESSED;
    }

    if(!c->connection_param)
        c->connection_param = calloc(1, sizeof(struct t_mpd_client_session));

    struct t_mpd_client_session *s = (struct t_mpd_client_session *)c->connection_param;

    if(mpd.conn_state != MPD_CONNECTED) {
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"disconnected\"}");
        mg_websocket_write(c, 1, mpd.buf, n);
    }
    else
    {
        mg_websocket_write(c, 1, mpd.buf, mpd.buf_size);

        if(s->song_id != mpd.song_id)
        {
            n = mpd_put_current_song(mpd.buf);
            mg_websocket_write(c, 1, mpd.buf, n);
            s->song_id = mpd.song_id;
        }

        if(s->queue_version != mpd.queue_version)
        {
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"update_playlist\"}");
            mg_websocket_write(c, 1, mpd.buf, n);
            s->queue_version = mpd.queue_version;
        }
    }

    return MG_REQUEST_PROCESSED;
}

void mpd_poll(struct mg_server *s)
{
    switch (mpd.conn_state) {
        case MPD_DISCONNECTED:
            /* Try to connect */
            fprintf(stdout, "MPD Connecting to %s:%d\n", mpd.host, mpd.port);
            mpd.conn = mpd_connection_new(mpd.host, mpd.port, 3000);
            if (mpd.conn == NULL) {
                fprintf(stderr, "Out of memory.");
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                mg_iterate_over_connections(s, mpd_notify_callback, 
                    (void *)mpd_connection_get_error_message(mpd.conn));
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if(mpd.password && !mpd_run_password(mpd.conn, mpd.password))
            {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                mg_iterate_over_connections(s, mpd_notify_callback, 
                    (void *)mpd_connection_get_error_message(mpd.conn));
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            fprintf(stderr, "MPD connected.\n");
            mpd.conn_state = MPD_CONNECTED;
            break;

        case MPD_FAILURE:
            fprintf(stderr, "MPD connection failed.\n");

        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if(mpd.conn != NULL)
                mpd_connection_free(mpd.conn);
            mpd.conn = NULL;
            mpd.conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            mpd.buf_size = mpd_put_state(mpd.buf, &mpd.song_id, &mpd.queue_version);
            mg_iterate_over_connections(s, mpd_notify_callback, NULL);
            break;
    }
}

char* mpd_get_title(struct mpd_song const *song)
{
    char *str, *ptr;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if(str == NULL){
        str = basename((char *)mpd_song_get_uri(song));
    }

    if(str == NULL)
        return NULL;

    ptr = str;
    while(*ptr++ != '\0')
        if(*ptr=='"')
            *ptr='\'';

    return str;
}

int mpd_put_state(char *buffer, int *current_song_id, unsigned *queue_version)
{
    struct mpd_status *status;
    int len;

    status = mpd_run_status(mpd.conn);
    if (!status) {
        fprintf(stderr, "MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
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

    song = mpd_run_current_song(mpd.conn);
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
    mpd_response_finish(mpd.conn);

    return cur - buffer;
}

int mpd_put_playlist(char *buffer)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;

    if (!mpd_send_list_queue_meta(mpd.conn)) {
        fprintf(stderr, "MPD mpd_send_list_queue_meta: %s\n", mpd_connection_get_error_message(mpd.conn));
        cur += snprintf(cur, end  - cur, "{\"type\":\"error\",\"data\":\"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
        return cur - buffer;
    }

    cur += snprintf(cur, end  - cur, "{\"type\": \"playlist\", \"data\": [ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
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

    if (!mpd_send_list_meta(mpd.conn, path)) {
        fprintf(stderr, "MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(mpd.conn));
        cur += snprintf(cur, end  - cur, "{\"type\":\"error\",\"data\":\"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
        return cur - buffer;
    }
    cur += snprintf(cur, end  - cur, "{\"type\":\"browse\",\"data\":[ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
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

    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    /* remove last ',' */
    cur--;
    cur += snprintf(cur, end  - cur, "] }");
    return cur - buffer;
}

void mpd_disconnect()
{
    mpd.conn_state = MPD_DISCONNECT;
    mpd_poll(NULL);
}