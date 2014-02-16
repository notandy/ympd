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

#include <string.h>

#include "http_server.h"
#include "http_files.h"

int callback_http(struct mg_connection *c)
{
    const struct embedded_file *req_file;

    if(!strcmp(c->uri, "/"))
        req_file = find_embedded_file("/index.html");
    else
        req_file = find_embedded_file(c->uri);

    if(req_file)
    {
        mg_send_header(c, "Content-Type", req_file->mimetype);
        mg_send_data(c, req_file->data, req_file->size);
    
        return MG_REQUEST_PROCESSED;
    }

    mg_send_status(c, 404);
    mg_printf_data(c, "Not Found");
    return MG_REQUEST_PROCESSED;
}
