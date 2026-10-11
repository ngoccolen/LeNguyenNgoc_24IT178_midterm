#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sort.h"

static const Options *sort_options;

static time_t
entry_seconds(const FileEntry *entry)
{
    switch (sort_options->time_mode) {
    case TIME_ATIME:
        return entry->st.st_atime;

    case TIME_CTIME:
        return entry->st.st_ctime;

    case TIME_MTIME:
    default:
        return entry->st.st_mtime;
    }
}

static long
entry_nanoseconds(const FileEntry *entry)
{
    switch (sort_options->time_mode) {
    case TIME_ATIME:
        return entry->st.st_atimensec;

    case TIME_CTIME:
        return entry->st.st_ctimensec;

    case TIME_MTIME:
    default:
        return entry->st.st_mtimensec;
    }
}

/* Compare timestamps: newer entries come first. */
static int
compare_time(const FileEntry *a, const FileEntry *b)
{
    time_t ta = entry_seconds(a);
    time_t tb = entry_seconds(b);
    long na;
    long nb;

    if (ta > tb)
        return -1;

    if (ta < tb)
        return 1;

    na = entry_nanoseconds(a);
    nb = entry_nanoseconds(b);

    if (na > nb)
        return -1;

    if (na < nb)
        return 1;

    return 0;
}

static int
compare_entries(const void *left, const void *right)
{
    const FileEntry *a = left;
    const FileEntry *b = right;
    int result = 0;

    switch (sort_options->sort_mode) {
    case SORT_SIZE:
        if (a->st.st_size > b->st.st_size)
            result = -1;
        else if (a->st.st_size < b->st.st_size)
            result = 1;
        break;

    case SORT_TIME:
        result = compare_time(a, b);
        break;

    case SORT_NAME:
    default:
        result = strcmp(a->name, b->name);
        break;
    }

    /* If primary keys tie, sort by name. */
    if (result == 0)
        result = strcmp(a->name, b->name);

    if (sort_options->reverse)
        result = result < 0 ? 1 : (result > 0 ? -1 : 0);

    return result;
}

void
sort_entries(FileList *list, const Options *opt)
{
    if (opt->force_unsorted ||
        opt->sort_mode == SORT_NONE ||
        list->count < 2)
        return;

    sort_options = opt;

    qsort(
        list->items,
        list->count,
        sizeof(list->items[0]),
        compare_entries
    );
}
