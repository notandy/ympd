#!/usr/bin/perl
# This program is used to embed arbitrary data into a C binary. It takes
# a list of files as an input, and produces a .c data file that contains
# contents of all these files as collection of char arrays.
#
# Usage: perl <this_file> <file1> [file2, ...] > embedded_data.c

use File::Basename;

%mimetypes = (
  js => 'application/javascript',
  css => 'text/css',
  ico => 'image/vnd.microsoft.icon',
  woff => 'application/font-woff',
  ttf => 'application/x-font-ttf',
  eot => 'application/octet-stream',
  svg => 'image/svg+xml',
  html => 'text/html'
);

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
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "src/http_server.h"

static const struct embedded_file embedded_files[] = {
EOS

foreach my $i (0 .. $#ARGV) {
  my ($ext) = $ARGV[$i] =~ /([^.]+)$/;
  my $mime = $mimetypes{$ext};
  $ARGV[$i] =~ s/htdocs//;
  print "  {\"$ARGV[$i]\", v$i, \"$mime\", sizeof(v$i) - 1},\n";
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

EOS
