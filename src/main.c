#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "http_server.h"
#include "mpd_client.h"

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
		"ympd-client",
		callback_ympd,
		sizeof(struct per_session_data__ympd),
		10,
	},

	{ NULL, NULL, 0, 0 } /* terminator */
};

int force_exit = 0;

void bye()
{
	force_exit = 1;
}

int main(int argc, char **argv)
{
	int n = 0;
	struct libwebsocket_context *context;
	int opts = 0;
	char interface_name[128] = "";
	const char *iface = NULL;
	struct lws_context_creation_info info;
	unsigned int oldus = 0;

	atexit(bye);
	memset(&info, 0, sizeof info);
	info.port = 7681;

	while (n >= 0) {
		n = getopt(argc, argv, "i:p:");
		if (n < 0)
			continue;
		switch (n) {
			case 'p':
			info.port = atoi(optarg);
			break;
			case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			iface = interface_name;
			break;
			case 'h':
			fprintf(stderr, "Usage: %s [OPTION]...\n"
				"[-=<p>] [--mpd-port=<P>] "
				"[-i <log bitfield>] "
				"[--resource_path <path>]\n", argv[0]);
			exit(1);
		}
	}

	info.iface = iface;
	info.protocols = protocols;
	info.extensions = libwebsocket_get_internal_extensions();
	info.gid = -1;
	info.uid = -1;
	info.ssl_cert_filepath = NULL;
	info.ssl_private_key_filepath = NULL;
	info.options = opts;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return EXIT_FAILURE;
	}

	n = 0;
	while (n >= 0 && !force_exit) {
		struct timeval tv;

		gettimeofday(&tv, NULL);

		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */

		if (((unsigned int)tv.tv_usec - oldus) > 500000) {
			mpd_loop();
			libwebsocket_callback_on_writable_all_protocol(&protocols[1]);
			oldus = tv.tv_usec;
		}

		n = libwebsocket_service(context, 50);
	}

	libwebsocket_context_destroy(context);
}