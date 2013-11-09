#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "http_server.h"
#include "mpd_client.h"

extern char *optarg;
extern char *resource_path;

struct libwebsocket_protocols protocols[] = {
    /* first protocol must always be HTTP handler */
    {
        "http-only",		/* name */
        callback_http,		/* callback */
        sizeof (struct per_session_data__http),	/* per_session_data_size */
        0,			/* max frame size / rx buffer */
        0,          /* no_buffer_all_partial_tx */
    },
    {
        "ympd-client",
        callback_ympd,
        sizeof(struct per_session_data__ympd),
        255,
        0,
    },

    { NULL, NULL, 0, 0, 0 } /* terminator */
};

int force_exit = 0;

void bye()
{
    force_exit = 1;
}

int main(int argc, char **argv)
{
    int n, gid = -1, uid = -1;
    struct libwebsocket_context *context;
    const char *cert_filepath = NULL;
    const char *private_key_filepath = NULL;
    const char *iface = NULL;
    struct lws_context_creation_info info;
    unsigned int oldus = 0;

    atexit(bye);
    memset(&info, 0, sizeof info);
    info.port = 80;

    while((n = getopt(argc, argv, "i:p:r:c:k:g:u:h")) != -1) {
        switch (n) {
            case 'i':
                iface = optarg;
                break;
            case 'p':
                info.port = atoi(optarg);
                break;
            case 'r':
                resource_path = optarg;
                break;
            case 'c':
                cert_filepath = optarg;
                break;
            case 'k':
                private_key_filepath = optarg;
                break;
            case 'g':
                gid = atoi(optarg);
                break;
            case 'u':
                uid = atoi(optarg);
                break;
            case '?':
            case 'h':
                lwsl_err("Usage: %s [OPTION]...\n"
                        "\t[-p <port>]\n"
                        "\t[-i <interface>]\n"
                        "\t[-r <htdocs path>]\n"
                        "\t[-c <ssl certificate filepath>]\n"
                        "\t[-k <ssl private key filepath>]\n"
                        "\t[-g <group id after socket bind>]\n"
                        "\t[-u <user id after socket bind>]\n"
                        "\t[-h]\n"
                        , argv[0]);
                return EXIT_FAILURE;
        }
    }

    if(cert_filepath != NULL && private_key_filepath == NULL) {
        lwsl_err("private key filepath needed\n");
        return EXIT_FAILURE;
    }

    if(private_key_filepath != NULL && cert_filepath == NULL) {
        lwsl_err("public cert filepath needed\n");
        return EXIT_FAILURE;
    }

    info.ssl_cert_filepath = cert_filepath;
    info.ssl_private_key_filepath = private_key_filepath;
    info.iface = iface;
    info.protocols = protocols;
    info.extensions = libwebsocket_get_internal_extensions();
    info.gid = gid;
    info.uid = uid;
    info.options = 0;

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

        if (((unsigned int)tv.tv_usec - oldus) > 1000 * 500) {
            mpd_loop();
            libwebsocket_callback_on_writable_all_protocol(&protocols[1]);
            oldus = tv.tv_usec;
        }

        n = libwebsocket_service(context, 50);
    }

    libwebsocket_context_destroy(context);
    return 0;
}
