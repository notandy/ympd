// Copyright (c) 2004-2013 Sergey Lyubka <valenok@gmail.com>
// Copyright (c) 2013 Cesanta Software Limited
// All rights reserved
//
// This library is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this library under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this library under a commercial
// license, as set out in <http://cesanta.com/products.html>.

// json encoder 'frozen' from https://github.com/cesanta/frozen

#include <stdio.h>

#include "json_encode.h"

int json_emit_int(char *buf, int buf_len, long int value) {
  return buf_len <= 0 ? 0 : snprintf(buf, buf_len, "%ld", value);
}

int json_emit_double(char *buf, int buf_len, double value) {
  return buf_len <= 0 ? 0 : snprintf(buf, buf_len, "%g", value);
}

int json_emit_quoted_str(char *buf, int buf_len, const char *str) {
  int i = 0, j = 0, ch;

#define EMIT(x) do { if (j < buf_len) buf[j++] = x; } while (0)

  EMIT('"');
  while ((ch = str[i++]) != '\0' && j < buf_len) {
    switch (ch) {
      case '"':  EMIT('\\'); EMIT('"'); break;
      case '\\': EMIT('\\'); EMIT('\\'); break;
      case '\b': EMIT('\\'); EMIT('b'); break;
      case '\f': EMIT('\\'); EMIT('f'); break;
      case '\n': EMIT('\\'); EMIT('n'); break;
      case '\r': EMIT('\\'); EMIT('r'); break;
      case '\t': EMIT('\\'); EMIT('t'); break;
      default: EMIT(ch);
    }
  }
  EMIT('"');
  EMIT(0);

  return j == 0 ? 0 : j - 1;
}

int json_emit_raw_str(char *buf, int buf_len, const char *str) {
  return buf_len <= 0 ? 0 : snprintf(buf, buf_len, "%s", str);
}