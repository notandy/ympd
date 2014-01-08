#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <libwebsockets.h>

struct per_session_data__http {
    int fd;
};
int callback_http(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len);

#endif

