/* This program is used to embed arbitrary data into a C binary. It takes
 * a list of files as an input, and produces a .c data file that contains
 * contents of all these files as collection of char arrays.
 *
 * Usage: ./mkdata <this_file> <file1> [file2, ...] > embedded_data.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <string.h>

const char* header =
"#include <stddef.h>\n"
"#include <string.h>\n"
"#include <sys/types.h>\n"
"#include \"src/http_server.h\"\n"
"\n"
"static const struct embedded_file embedded_files[] = {\n";

const char* footer =
"  {NULL, NULL, NULL, 0}\n"
"};\n"
"\n"
"const struct embedded_file *find_embedded_file(const char *name) {\n"
"  const struct embedded_file *p;\n"
"  for (p = embedded_files; p->name != NULL; p++)\n"
"    if (!strcmp(p->name, name))\n"
"      return p;\n"
"  return NULL;\n"
"}\n";

static const char* get_mime(char* filename)
{
    const char *extension = strrchr(filename, '.');
    if(!strcmp(extension, ".js"))
        return "application/javascript";
    if(!strcmp(extension, ".css"))
        return "text/css";
    if(!strcmp(extension, ".ico"))
        return "image/vnd.microsoft.icon";
    if(!strcmp(extension, ".woff"))
        return "application/font-woff";
    if(!strcmp(extension, ".ttf"))
        return "application/x-font-ttf";
    if(!strcmp(extension, ".eot"))
        return "application/octet-stream";
    if(!strcmp(extension, ".svg"))
        return "image/svg+xml";
    if(!strcmp(extension, ".html"))
        return "text/html";
    return "text/plain";
}

int main(int argc, char *argv[])
{
    int i, j, buf;
    FILE *fd;

    if(argc <= 1)
        error(EXIT_FAILURE, 0, "Usage: ./%s <this_file> <file1> [file2, ...] > embedded_data.c", argv[0]);

    for(i = 1; i < argc; i++)
    {
        printf("static const unsigned char v%d[] = {", i);
        
        fd = fopen(argv[i], "r");
        if(!fd)
            error(EXIT_FAILURE, errno, "Failed open file %s", argv[i]);

        j = 0;
        while((buf = fgetc(fd)) != EOF)
        {
            if(!(j % 12))
                putchar('\n');

            printf(" %#04x, ", buf);
            j++;
        }
        printf(" 0x00\n};\n\n");
        fclose(fd);
    }
    fputs(header, stdout);

    for(i = 1; i < argc; i++)
    {
        printf("  {\"%s\", v%d, \"%s\", sizeof(v%d) - 1}, \n", 
            argv[i]+6, i, get_mime(argv[i]), i);
    }

    fputs(footer, stdout);
    return EXIT_SUCCESS;
}
