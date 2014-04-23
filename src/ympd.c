/* ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
    char *run_as_user = NULL;

    atexit(bye);
    mg_set_option(server, "listening_port", "8080");
    mpd.port = 6600;
    strcpy(mpd.host, "127.0.0.1");

    static struct option long_options[] = {
        {"host",         required_argument, 0, 'h'},
        {"port",         required_argument, 0, 'p'},
        {"webport",      required_argument, 0, 'w'},
        {"user",         required_argument, 0, 'u'},
        {"version",      no_argument,       0, 'v'},
        {"help",         no_argument,       0,  0 },
        {0,              0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "h:p:w:u:v",
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
                run_as_user = strdup(optarg);
                break;
            case 'v':
                fprintf(stdout, "ympd  %d.%d.%d\n"
                        "Copyright (C) 2014 Andrew Karpow <andy@ndyk.de>\n"
                        "built " __DATE__ " "__TIME__ " ("__VERSION__")\n",
                        YMPD_VERSION_MAJOR, YMPD_VERSION_MINOR, YMPD_VERSION_PATCH);
                return EXIT_SUCCESS;
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n\n"
                        " -h, --host <host>\t\tconnect to mpd at host [localhost]\n"
                        " -p, --port <port>\t\tconnect to mpd at port [6600]\n"
                        " -w, --webport [ip:]<port>\tlisten interface/port for webserver [8080]\n"
                        " -u, --user <username>\t\tdrop priviliges to user after socket bind\n"
                        " -V, --version\t\t\tget version\n"
                        " --help\t\t\t\tthis help\n"
                        , argv[0]);
                return EXIT_FAILURE;
        }
    }

    /* drop privilges at last to ensure proper port binding */
    if(run_as_user != NULL)
    {
        mg_set_option(server, "run_as_user", run_as_user);
        free(run_as_user);
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
