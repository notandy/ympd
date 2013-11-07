#include <libwebsockets.h>
#include <stdio.h>
#include <string.h>
#include "http_server.h"

#define INSTALL_DATADIR "/home/andy/workspace/ympd"
#define LOCAL_RESOURCE_PATH INSTALL_DATADIR"/htdocs"
char *resource_path = LOCAL_RESOURCE_PATH;

struct serveable {
	const char *urlpath;
	const char *mimetype;
}; 

static const struct serveable whitelist[] = {
	{ "/css/bootstrap.css", "text/css" },
	{ "/css/slider.css", "text/css" },
	{ "/css/mpd.css", "text/css" },

	{ "/js/bootstrap.min.js", "text/javascript" },
	{ "/js/mpd.js", "text/javascript" },
	{ "/js/jquery-1.10.2.min.js", "text/javascript" },
	{ "/js/bootstrap-slider.js", "text/javascript" },
	{ "/js/sammy.js", "text/javascript" },
	
	{ "/fonts/glyphicons-halflings-regular.woff", "application/x-font-woff"},
	{ "/fonts/glyphicons-halflings-regular.svg", "image/svg+xml"},
	{ "/fonts/glyphicons-halflings-regular.ttf", "application/x-font-ttf"},
	{ "/fonts/glyphicons-halflings-regular.eot", "application/vnd.ms-fontobject"},

	/* last one is the default served if no match */
	{ "/index.html", "text/html" },
};

int callback_http(struct libwebsocket_context *context,
	struct libwebsocket *wsi,
	enum libwebsocket_callback_reasons reason, void *user,
	void *in, size_t len)
{
	char buf[256];
	int n;

	switch (reason) {
		case LWS_CALLBACK_HTTP:
		for (n = 0; n < (sizeof(whitelist) / sizeof(whitelist[0]) - 1); n++)
		{
			if (in && strcmp((const char *)in, whitelist[n].urlpath) == 0)
				break;
		}
		sprintf(buf, "%s%s", resource_path, whitelist[n].urlpath);

		if (libwebsockets_serve_http_file(context, wsi, buf, whitelist[n].mimetype))
			return -1; /* through completion or error, close the socket */

			break;

		case LWS_CALLBACK_HTTP_FILE_COMPLETION:
			/* kill the connection after we sent one file */
			return -1;
		default:
			break;
	}

	return 0;
}
