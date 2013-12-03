#include <libwebsockets.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "http_server.h"
#include "mpd_client.h"

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

    { "assets/favicon.ico", "image/vnd.microsoft.icon" },

    /* last one is the default served if no match */
    { "/index.html", "text/html" },
};

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') {
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

int callback_http(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len)
{
    char buf[256], *response_buffer, *p;
    size_t n, response_size;

    switch (reason) {
        case LWS_CALLBACK_HTTP:
            if(in && strncmp((const char *)in, "/api/", 5) == 0)
            {
                response_buffer = (char *)malloc(100 * 1024 + 100);
                p = response_buffer;

                /* put content length and payload to buffer */
                if(strncmp((const char *)in, "/api/get_browse", 15) == 0)
                {
                    char *url;
                    if(sscanf(in, "/api/get_browse/%m[^\t\n]", &url))
                    {
                        char *url_decoded = url_decode(url);
                        printf("searching for %s", url_decoded);
                        response_size = mpd_put_browse(response_buffer + 98, url_decoded);
                        free(url_decoded);
                        free(url);
                    }
                    else
                        response_size = mpd_put_browse(response_buffer + 98, "/");

                }
                else if(strncmp((const char *)in, "/api/get_playlist", 17)  == 0)
                    response_size = mpd_put_playlist(response_buffer + 98);
                else
                {
                    /* invalid request, close connection */
                    free(response_buffer);
                    return -1;
                }
                p += response_size + sprintf(p, "HTTP/1.0 200 OK\x0d\x0a"
                                "Server: libwebsockets\x0d\x0a"
                                "Content-Type: application/json\x0d\x0a"
                                "Content-Length: %6lu\x0d\x0a\x0d\x0a", 
                                response_size
                );
                response_buffer[98] = '{';

                n = libwebsocket_write(wsi, (unsigned char *)response_buffer,
                    p - response_buffer, LWS_WRITE_HTTP);

                free(response_buffer);
                /*
                 * book us a LWS_CALLBACK_HTTP_WRITEABLE callback
                 */
                libwebsocket_callback_on_writable(context, wsi);

            }
            else if(in && strcmp((const char *)in, "getPlaylist") == 0)
            {
                
            }
            else
            {            
                for (n = 0; n < (sizeof(whitelist) / sizeof(whitelist[0]) - 1); n++)
                {
                    if (in && strcmp((const char *)in, whitelist[n].urlpath) == 0)
                        break;
                }
                sprintf(buf, "%s%s", resource_path, whitelist[n].urlpath);

                if (libwebsockets_serve_http_file(context, wsi, buf, whitelist[n].mimetype))
                    return -1; /* through completion or error, close the socket */
            }
            break;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            /* kill the connection after we sent one file */
            return -1;
        default:
            break;
    }

    return 0;
}
