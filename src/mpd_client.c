
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

struct mpd_connection *conn = NULL;
enum mpd_conn_states mpd_conn_state = MPD_DISCONNECTED;
enum mpd_state mpd_play_state = MPD_STATE_UNKNOWN;

callback_ympd(struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len)
{
	int n, m;
	/*unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 +
		LWS_SEND_BUFFER_POST_PADDING];*/
	char *buf;
	struct per_session_data__ympd *pss = (struct per_session_data__ympd *)user;

	//memset(buf, 0, sizeof(buf));

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_dumb_increment: "
			"LWS_CALLBACK_ESTABLISHED\n");
		break;

		case LWS_CALLBACK_SERVER_WRITEABLE:

		if(pss->do_send & DO_SEND_STATE)
			n = mpd_put_state(buf);
		else if(pss->do_send & DO_SEND_PLAYLIST)
		{
			printf("Before buf is %ud\n",buf);
			n = mpd_put_playlist(&buf);
			printf("n is %d, buf is %ud\n", n, buf);
			pss->do_send &= ~DO_SEND_PLAYLIST;
		}
		else if(pss->do_send & DO_SEND_TRACK_INFO)
			n = mpd_put_current_song(buf);
		else
			return 0;

		m = libwebsocket_write(wsi, buf, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to socket\n", n);
			return -1;
		}
		break;

		case LWS_CALLBACK_RECEIVE:
		if(!strcmp((const char *)in, MPD_API_GET_STATE))
			pss->do_send |= DO_SEND_STATE;
		if(!strcmp((const char *)in, MPD_API_GET_PLAYLIST))
			pss->do_send |= DO_SEND_PLAYLIST;

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

		conn = mpd_connection_new("127.0.0.1", 6600, 3000);
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

int mpd_put_state(char* buffer)
{
	struct mpd_status *status;

	status = mpd_run_status(conn);
	if (!status) {
		lwsl_notice("MPD Status: %s", mpd_connection_get_error_message(conn));
		mpd_conn_state = MPD_FAILURE;
		return;
	}

	sprintf(buffer, 
		"{\"type\": \"state\", \"data\": {"
		" \"state\":%d, \"volume\":%d, \"repeat\":%d,"
        " \"single\":%d, \"consume\":%d, \"random\":%d, "
        " \"songpos\": %d, \"elapsedTime\": %d, \"totalTime\":%d"
	    "}}", 
		mpd_status_get_state(status),
		mpd_status_get_volume(status), 
		mpd_status_get_repeat(status),
		mpd_status_get_single(status),
		mpd_status_get_consume(status),
		mpd_status_get_random(status),
		mpd_status_get_song_pos(status),
		mpd_status_get_elapsed_time(status),
		mpd_status_get_total_time(status));

	mpd_status_free(status);
	mpd_response_finish(conn);
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

int mpd_put_playlist(char** buffer)
{
	mpd_send_list_queue_range_meta(conn, 0, 10);
	int max_size = 1024*200;
	*buffer = (char **)malloc(max_size + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);
	printf("Buffer init: %p %ud\n", *buffer, *buffer);

	char *p = &((*buffer)[LWS_SEND_BUFFER_PRE_PADDING]);

	struct mpd_entity *entity;
	for(entity = mpd_recv_entity(conn); entity; entity = mpd_recv_entity(conn)) {

		if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			struct mpd_song* song = mpd_entity_get_song(entity);
			p += snprintf(p, max_size, "ID: %d, Song: %s\n", mpd_song_get_id(song), mpd_song_get_uri(song));
		}

		if(entity != NULL)
			mpd_entity_free(entity);
	}

	printf("strlen is %d\n", strlen(p));
	return strlen(*buffer + LWS_SEND_BUFFER_PRE_PADDING) +1;
}