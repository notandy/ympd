#include <libwebsockets.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
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
    int option_index = 0;
    unsigned int oldus = 0;
    struct libwebsocket_context *context;
    struct lws_context_creation_info info;
    const char *cert_filepath = NULL;
    const char *private_key_filepath = NULL;
    const char *iface = NULL;

    atexit(bye);
    memset(&info, 0, sizeof info);
    info.port = 8080;
    mpd_host = "127.0.0.1";
    mpd_port = 6600;
    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

    static struct option long_options[] = {
        {"host",         required_argument, 0, 'h'},
        {"port",         required_argument, 0, 'p'},
        {"interface",    required_argument, 0, 'i'},
        {"webport",      required_argument, 0, 'w'},
        {"resourcepath", required_argument, 0, 'r'},
        {"ssl_cert",     required_argument, 0, 'c'},
        {"ssl_key",      required_argument, 0, 'k'},
        {"gid",          required_argument, 0, 'g'},
        {"uid",          required_argument, 0, 'u'},
        {"verbose",      optional_argument, 0, 'v'},
        {"help",         no_argument,       0,  0 },
        {0,              0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "h:p:i:r:c:k:g:uv::W",
                long_options, &option_index)) != -1) {
        switch (n) {
            case 'h':
                mpd_host = optarg;
                break;
            case 'p':
                mpd_port = atoi(optarg);
            case 'i':
                iface = optarg;
                break;
            case 'w':
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
            case 'v':
                if(optarg)
                    lws_set_log_level(strtol(optarg, NULL, 10), NULL);
                else
                    lws_set_log_level(LLL_ERR | LLL_WARN | 
                            LLL_NOTICE | LLL_INFO, NULL);
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n\n"
                        "\t-h, --host <host>\t\tconnect to mpd at host [localhost]\n"
                        "\t-p, --port <port>\t\tconnect to mpd at port [6600]\n"
                        "\t-i, --interface <interface>\tlisten interface for webserver [all]\n"
                        "\t-w, --webport <port>\t\tlisten port for webserver [8080]\n"
                        "\t-r, --resourcepath <path>\tresourcepath for webserver [" LOCAL_RESOURCE_PATH "]\n"
                        "\t-c, --ssl_cert <filepath>\tssl certificate ssl_private_key_filepath\n"
                        "\t-k, --ssl_key <filepath>\tssl private key filepath\n"
                        "\t-u, --uid <id>\t\t\tuser id after socket bind\n"
                        "\t-g, --gid <id>\t\t\tgroup id after socket bind\n"
                        "\t-v, --verbose[<level>]\t\tverbosity level\n"
                        "\t--help\t\t\t\tthis help\n"
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
