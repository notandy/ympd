
#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/song.h>
#include <mpd/entity.h>
#include <mpd/search.h>
#include <mpd/tag.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "mpd_client.h"

#define MAX_SIZE 9000*10

struct mpd_connection *conn = NULL;
enum mpd_conn_states mpd_conn_state = MPD_DISCONNECTED;
enum mpd_state mpd_play_state = MPD_STATE_UNKNOWN;
static int global_send;

callback_ympd(struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len)
{
	int n, m;
	char *buf = NULL, *p;
	struct per_session_data__ympd *pss = (struct per_session_data__ympd *)user;

	//if(global_send || (pss != NULL && pss->do_send))
	//{
		buf = (char *)malloc(MAX_SIZE + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);

		if(buf == NULL)
			return -1;

		p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	//}

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED:
			lwsl_info("callback_dumb_increment: "
				"LWS_CALLBACK_ESTABLISHED\n");
			break;

		case LWS_CALLBACK_SERVER_WRITEABLE:
			if(pss->do_send & DO_SEND_STATE)
			{
				n = mpd_put_state(p);
				pss->do_send &= ~DO_SEND_STATE;
			}
			else if(pss->do_send & DO_SEND_PLAYLIST)
			{
				n = mpd_put_playlist(p);
				pss->do_send &= ~DO_SEND_PLAYLIST;
			}
			else if(pss->do_send & DO_SEND_TRACK_INFO)
				n = mpd_put_current_song(p);
			else
			{
				n = mpd_put_state(p);
			}

			m = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
			if (m < n) {
				lwsl_err("ERROR %d writing to socket\n", n);
				return -1;
			}
			break;

		case LWS_CALLBACK_RECEIVE:
			printf("Got %s\n", (char *)in);

			if(!strcmp((const char *)in, MPD_API_GET_STATE))
				pss->do_send |= DO_SEND_STATE;
			else if(!strcmp((const char *)in, MPD_API_GET_PLAYLIST))
				pss->do_send |= DO_SEND_PLAYLIST;
			else if(!strcmp((const char *)in, MPD_API_SET_PAUSE))
			{
				mpd_send_toggle_pause(conn);
				mpd_response_finish(conn);
			}
			else if(!strcmp((const char *)in, MPD_API_SET_PREV))
			{
				mpd_send_previous(conn);
				mpd_response_finish(conn);
			}
			else if(!strcmp((const char *)in, MPD_API_SET_NEXT))
			{
				mpd_send_next(conn);
				mpd_response_finish(conn);
			}
			else if(!strncmp((const char *)in, MPD_API_SET_VOLUME, sizeof(MPD_API_SET_VOLUME)-1))
			{
				unsigned int volume;
				if(sscanf(in, "MPD_API_SET_VOLUME,%ud", &volume) && volume < 100)
				{
					printf("Setting volume to %d\n", volume);
					mpd_run_set_volume(conn, volume);

				}
			}



		break;
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	 case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		/* you could return non-zero here and kill the connection */
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

		conn = mpd_connection_new("10.23.44.2", 6600, 3000);
		if (conn == NULL) {
			lwsl_err("%s", "Out of memory");
			mpd_conn_state = MPD_FAILURE;
			return;
		}

		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			lwsl_notice("MPD Connection: %s", mpd_connection_get_error_message(conn));
			mpd_conn_state = MPD_FAILURE;
			return;
		}

		mpd_conn_state = MPD_CONNECTED;
		break;

		case MPD_FAILURE:
		if(conn != NULL)
			mpd_connection_free(conn);
		conn = NULL;
		mpd_conn_state = MPD_DISCONNECTED;
		break;

		case MPD_CONNECTED:
			printf("mpd connected\n");
	}


}

const char* encode_string(const char *str)
{
	char *ptr = (char *)str;
	while(*ptr++ != '\0')
		if(*ptr=='"')
			*ptr=' ';
	return str;
}

int mpd_put_state(char* buffer)
{
	struct mpd_status *status;
	int len;

	status = mpd_run_status(conn);
	if (!status) {
		lwsl_notice("MPD Status: %s", mpd_connection_get_error_message(conn));
		mpd_conn_state = MPD_FAILURE;
		return;
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

	printf("buffer: %s\n", buffer);
	mpd_status_free(status);
	//printf("status: %d\n", mpd_response_finish(conn));
	return len;
}

int mpd_put_current_song(char* buffer)
{
	struct mpd_song *song;

	song = mpd_run_current_song(conn);
	if (song != NULL) {
		sprintf(buffer, 
			"{\"type\": \"current_song\", \"data\": {"
			" \"uri\":%s"
		    "}}", 
			mpd_song_get_uri(song)
		);
		mpd_song_free(song);
	}
	mpd_response_finish(conn);
}

int mpd_put_playlist(char* buffer)
{
	char *cur = buffer;
	const char *end = buffer + MAX_SIZE;
	struct mpd_entity *entity;
	struct mpd_song const *song;

	mpd_send_list_queue_meta(conn);

	cur += snprintf(cur, end  - cur, "{\"type\": \"playlist\", \"data\": [ ");

	for(entity = mpd_recv_entity(conn); entity; entity = mpd_recv_entity(conn)) {

		if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			song = mpd_entity_get_song(entity);
			cur += snprintf(cur, end  - cur, 
				"{\"id\":%d, \"uri\":\"%s\", \"duration\":%d, \"title\":\"%s\"},",
				mpd_song_get_id(song),
				mpd_song_get_uri(song),
				mpd_song_get_duration(song),
				encode_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0))
			);
		}

		mpd_entity_free(entity);
	}
	//printf("status: %d\n", mpd_response_finish(conn));

	/* remove last ',' */
	cur--;
	cur += snprintf(cur, end  - cur, "] }");
	printf("buffer: %s\n", buffer);
	return cur - buffer;
}