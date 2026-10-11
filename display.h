#ifndef DISPLAY_H
#define DISPLAY_H

#include <sys/stat.h>

#include "filesystem.h"
#include "options.h"

void display_list(const FileList *list, const Options *opt);
void display_operand(const char *name, const char *path,
    const struct stat *st, const Options *opt);
void display_header(const char *path, const Options *opt, int already_printed);

#endif
