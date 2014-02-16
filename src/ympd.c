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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <pthread.h>

#include "mongoose.h"
#include "http_server.h"
#include "mpd_client.h"
#include "config.h"

extern char *optarg;

int force_exit = 0;

void bye()
{
    force_exit = 1;
}

static int server_callback(struct mg_connection *c) {

    //printf("Got REQ: (%lu) WS:%d\n", c->content_len, c->is_websocket);
    //fwrite(c->content, c->content_len, 1, stdout);
    //printf("\n");

    if (c->is_websocket)
    {
        c->content[c->content_len] = '\0';
        if(c->content_len)
            return callback_mpd(c);
        else
            return MG_CLIENT_CONTINUE;
    }
    else
        return callback_http(c);
}

int main(int argc, char **argv)
{
    int n, option_index = 0;
    struct mg_server *server = mg_create_server(NULL);
    unsigned int current_timer = 0, last_timer = 0;

    atexit(bye);
    mg_set_option(server, "listening_port", "8080");
    mpd.port = 6600;
    strcpy(mpd.host, "127.0.0.1");

    static struct option long_options[] = {
        {"host",         required_argument, 0, 'h'},
        {"port",         required_argument, 0, 'p'},
        {"webport",      required_argument, 0, 'w'},
        {"user",         required_argument, 0, 'u'},
        {"version",      no_argument,       0, 'V'},
        {"help",         no_argument,       0,  0 },
        {0,              0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "h:p:w:u::V",
                long_options, &option_index)) != -1) {
        switch (n) {
            case 'h':
                strncpy(mpd.host, optarg, sizeof(mpd.host));
                break;
            case 'p':
                mpd.port = atoi(optarg);
            case 'w':
                mg_set_option(server, "listening_port", optarg);
                break;
            case 'u':
                mg_set_option(server, "run_as_user", optarg);
                break;
            case 'V':
                fprintf(stdout, "ympd  %d.%d.%d\n"
                        "Copyright (C) 2014 Andrew Karpow <andy@ndyk.de>\n"
                        "built " __DATE__ " "__TIME__ " ("__VERSION__")\n",
                        YMPD_VERSION_MAJOR, YMPD_VERSION_MINOR, YMPD_VERSION_PATCH);
                return EXIT_SUCCESS;
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n\n"
                        "\t-h, --host <host>\t\tconnect to mpd at host [localhost]\n"
                        "\t-p, --port <port>\t\tconnect to mpd at port [6600]\n"
                        "\t-w, --webport [ip:]<port>\t\tlisten interface/port for webserver [8080]\n"
                        "\t-u, --user <username>\t\t\tdrop priviliges to user after socket bind\n"
                        "\t-V, --version\t\t\tget version\n"
                        "\t--help\t\t\t\tthis help\n"
                        , argv[0]);
                return EXIT_FAILURE;
        }
    }

    mg_set_http_close_handler(server, mpd_close_handler);
    mg_set_request_handler(server, server_callback);
    while (!force_exit) {
        current_timer = mg_poll_server(server, 200);
        if(current_timer - last_timer)
        {
            last_timer = current_timer;
            mpd_poll(server);
        }
    }

    mpd_disconnect();
    mg_destroy_server(&server);

    return EXIT_SUCCESS;
}
