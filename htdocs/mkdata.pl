# This program is used to embed arbitrary data into a C binary. It takes
# a list of files as an input, and produces a .c data file that contains
# contents of all these files as collection of char arrays.
#
# Usage: perl <this_file> <file1> [file2, ...] > embedded_data.c

use File::MimeInfo;

foreach my $i (0 .. $#ARGV) {
  open FD, '<:raw', $ARGV[$i] or die "Cannot open $ARGV[$i]: $!\n";
  printf("static const unsigned char v%d[] = {", $i);
  my $byte;
  my $j = 0;
  while (read(FD, $byte, 1)) {
    if (($j % 12) == 0) {
      print "\n";
    }
    printf ' %#04x,', ord($byte);
    $j++;
  }
  print " 0x00\n};\n";
  close FD;
}

print <<EOS;
/* ympd
   (c) 2013-2014 Andrew Karpow <andy\@ympd.org>
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

#ifndef __HTTP_FILES_H__
#define __HTTP_FILES_H__

#include <stddef.h>
#include <string.h>
#include <sys/types.h>

static const struct embedded_file {
  const char *name;
  const unsigned char *data;
  const char *mimetype;
  size_t size;
} embedded_files[] = {
EOS

foreach my $i (0 .. $#ARGV) {
  my $mime = mimetype($ARGV[$i]);
  print "  {\"/$ARGV[$i]\", v$i, \"$mime\", sizeof(v$i) - 1},\n";
}

print <<EOS;
  {NULL, NULL, NULL, 0}
};

const struct embedded_file *find_embedded_file(const char *name) {
  const struct embedded_file *p;
  for (p = embedded_files; p->name != NULL; p++)
    if (!strcmp(p->name, name))
      return p;
  return NULL;
}
#endif

EOS
