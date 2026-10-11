#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesystem.h"

static char *
copy_string(const char *src)
{
    size_t len = strlen(src) + 1;
    char *dest = malloc(len);

    if (dest == NULL)
        return NULL;

    memcpy(dest, src, len);
    return dest;
}

static char *
join_path(const char *directory, const char *name)
{
    size_t dir_len = strlen(directory);
    size_t name_len = strlen(name);
    int need_slash = dir_len > 0 && directory[dir_len - 1] != '/';
    char *result = malloc(dir_len + (size_t)need_slash + name_len + 1);
    size_t pos;

    if (result == NULL)
        return NULL;

    memcpy(result, directory, dir_len);
    pos = dir_len;
    if (need_slash)
        result[pos++] = '/';
    memcpy(result + pos, name, name_len);
    result[pos + name_len] = '\0';
    return result;
}

static int
should_show(const char *name, const Options *opt)
{
    if (opt->all)
        return 1;

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return 0;

    if (name[0] == '.')
        return opt->almost_all;

    return 1;
}

static int
append_entry(FileList *list, const char *name, const char *path,
    const struct stat *st)
{
    FileEntry *new_items;
    char *name_copy;
    char *path_copy;
    size_t new_capacity;

    if (list->count == list->capacity) {
        new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        if (new_capacity < list->capacity ||
            new_capacity > ((size_t)-1) / sizeof(*list->items)) {
            errno = ENOMEM;
            return -1;
        }

        new_items = realloc(list->items, new_capacity * sizeof(*list->items));
        if (new_items == NULL)
            return -1;

        list->items = new_items;
        list->capacity = new_capacity;
    }

    name_copy = copy_string(name);
    if (name_copy == NULL)
        return -1;

    path_copy = copy_string(path);
    if (path_copy == NULL) {
        free(name_copy);
        return -1;
    }

    list->items[list->count].name = name_copy;
    list->items[list->count].path = path_copy;
    list->items[list->count].st = *st;
    list->count++;
    return 0;
}

int
read_directory(const char *path, const Options *opt, FileList *list)
{
    DIR *dir;
    struct dirent *entry;
    int failed = 0;

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;

    dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return -1;
    }

    for (;;) {
        char *full_path;
        struct stat st;

        errno = 0;
        entry = readdir(dir);
        if (entry == NULL) {
            if (errno != 0) {
                perror(path);
                failed = 1;
            }
            break;
        }

        if (!should_show(entry->d_name, opt))
            continue;

        full_path = join_path(path, entry->d_name);
        if (full_path == NULL) {
            perror("ls: allocate path");
            failed = 1;
            break;
        }

        if (lstat(full_path, &st) == -1) {
            perror(full_path);
            free(full_path);
            failed = 1;
            continue;
        }

        if (append_entry(list, entry->d_name, full_path, &st) == -1) {
            perror("ls: allocate entry");
            free(full_path);
            failed = 1;
            break;
        }

        free(full_path);
    }

    if (closedir(dir) == -1) {
        perror(path);
        failed = 1;
    }

    return failed ? -1 : 0;
}

void
free_filelist(FileList *list)
{
    size_t i;

    for (i = 0; i < list->count; i++) {
        free(list->items[i].name);
        free(list->items[i].path);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
