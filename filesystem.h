#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>
#include <sys/stat.h>

#include "options.h"

typedef struct {
    char *name;
    char *path;
    struct stat st;             /* Metadata of the entry itself (lstat). */
} FileEntry;

typedef struct {
    FileEntry *items;
    size_t count;
    size_t capacity;
} FileList;

int read_directory(const char *path, const Options *opt, FileList *list);
void free_filelist(FileList *list);

#endif
