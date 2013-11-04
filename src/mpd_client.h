#include <libwebsockets.h>

#define DO_SEND_STATE      (1 << 0)
#define DO_SEND_PLAYLIST   (1 << 1)
#define DO_SEND_TRACK_INFO (1 << 2)

struct per_session_data__ympd {
	int do_send;
};

enum mpd_conn_states {
	MPD_FAILURE,
	MPD_DISCONNECTED,
	MPD_CONNECTED,
	MPD_PLAYING
};

#define MPD_API_GET_STATE        "MPD_API_GET_STATE"
#define MPD_API_GET_SEEK         "MPD_API_GET_SEEK"
#define MPD_API_GET_PLAYLIST     "MPD_API_GET_PLAYLIST"
#define MPD_API_GET_TRACK_INFO   "MPD_API_GET_TRACK_INFO"
#define MPD_API_ADD_TRACK        "MPD_API_GET_STATE"
#define MPD_API_SET_VOLUME       "MPD_API_SET_VOLUME"
#define MPD_API_SET_PAUSE        "MPD_API_SET_PAUSE"
#define MPD_API_SET_PLAY         "MPD_API_SET_PLAY"
#define MPD_API_SET_STOP         "MPD_API_SET_STOP"
#define MPD_API_SET_SEEK         "MPD_API_SET_SEEK"


int callback_ympd(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
			void *user, void *in, size_t len);

void mpd_connect();
int mpd_put_state(char* buffer);
int mpd_put_current_song(char* buffer);
int mpd_put_playlist(char** buffer);
