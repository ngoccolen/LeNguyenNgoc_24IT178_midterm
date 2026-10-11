#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>

#if defined(__linux__)
#include <sys/sysmacros.h>
#endif

#include "display.h"

#ifndef S_ISVTX
#define S_ISVTX 01000
#endif

static char
file_type(mode_t mode)
{
    if (S_ISREG(mode))  return '-';
    if (S_ISDIR(mode))  return 'd';
    if (S_ISLNK(mode))  return 'l';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    if (S_ISCHR(mode))  return 'c';
    if (S_ISBLK(mode))  return 'b';
#ifdef S_ISWHT
    if (S_ISWHT(mode))  return 'w';
#endif
    return '?';
}

static void
format_mode(mode_t mode, char out[11])
{
    out[0] = file_type(mode);
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID)
        out[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        out[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        out[9] = (mode & S_IXOTH) ? 't' : 'T';
    out[10] = '\0';
}

static void
print_name(const char *name, const Options *opt)
{
    const unsigned char *p;
    NameMode mode = opt->name_mode;

    if (mode == NAME_AUTO)
        mode = isatty(STDOUT_FILENO) ? NAME_QUESTION : NAME_RAW;

    if (mode == NAME_RAW) {
        fputs(name, stdout);
        return;
    }

    for (p = (const unsigned char *)name; *p != '\0'; p++) {
        if (isprint(*p))
            putchar(*p);
        else
            putchar('?');
    }
}

static char
classify_suffix(const struct stat *st)
{
    mode_t mode = st->st_mode;

    if (S_ISDIR(mode)) return '/';
    if (S_ISLNK(mode)) return '@';
    if (S_ISFIFO(mode)) return '|';
    if (S_ISSOCK(mode)) return '=';
#ifdef S_ISWHT
    if (S_ISWHT(mode)) return '%';
#endif
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        return '*';
    return '\0';
}

static void
print_name_suffix(const char *name, const struct stat *st, const Options *opt)
{
    char suffix = opt->classify ? classify_suffix(st) : '\0';
    print_name(name, opt);
    if (suffix != '\0')
        putchar(suffix);
}

static void
human_size(uintmax_t bytes, char *buffer, size_t buffer_size)
{
    static const char units[] = "BKMGTPE";
    double value = (double)bytes;
    size_t unit = 0;

    while (value >= 1024.0 && unit < sizeof(units) - 1) {
        value /= 1024.0;
        unit++;
    }

    if (unit == 0)
        (void)snprintf(buffer, buffer_size, "%juB", bytes);
    else
        (void)snprintf(buffer, buffer_size, "%.1f%c", value, units[unit]);
}

static uintmax_t
parse_blocksize_env(void)
{
    const char *env = getenv("BLOCKSIZE");
    char *end;
    unsigned long long value;
    unsigned int power = 0;

    if (env == NULL || *env == '\0')
        return 512;

    errno = 0;
    value = strtoull(env, &end, 10);
    if (errno != 0 || end == env || value == 0)
        return 512;

    if (*end != '\0') {
        int suffix = tolower((unsigned char)*end++);
        if (suffix == 'k') power = 1;
        else if (suffix == 'm') power = 2;
        else if (suffix == 'g') power = 3;
        else if (suffix == 't') power = 4;
        else return 512;
        if (*end == 'b' || *end == 'B') end++;
        if (*end != '\0') return 512;
    }

    while (power-- > 0) {
        if (value > ULLONG_MAX / 1024ULL)
            return 512;
        value *= 1024ULL;
    }

    return (uintmax_t)value;
}

static uintmax_t
block_unit(const Options *opt)
{
    if (opt->size_mode == SIZE_KB)
        return 1024;
    if (opt->size_mode == SIZE_HUMAN)
        return 0;
    return parse_blocksize_env();
}

/* -k applies to -s; -h also changes the long-format size field. */
static uintmax_t
total_block_unit(const Options *opt)
{
    if (opt->blocks)
        return block_unit(opt);
    if (opt->long_human)
        return 0;
    return parse_blocksize_env();
}

static uintmax_t
allocated_bytes(const struct stat *st)
{
    if (st->st_blocks <= 0)
        return 0;
    return (uintmax_t)st->st_blocks * 512U;
}

static uintmax_t
round_up_units(uintmax_t bytes, uintmax_t unit)
{
    if (unit == 0)
        return 0;
    return bytes / unit + (bytes % unit != 0);
}

static void
print_block_prefix(const struct stat *st, const Options *opt)
{
    uintmax_t bytes;
    uintmax_t unit;
    char buffer[64];

    if (!opt->blocks)
        return;

    /* NetBSD -h changes -s to report the file's byte size human-readably,
       rather than allocated filesystem blocks. */
    bytes = (opt->size_mode == SIZE_HUMAN)
        ? (st->st_size < 0 ? 0 : (uintmax_t)st->st_size)
        : allocated_bytes(st);
    unit = block_unit(opt);
    if (unit == 0) {
        human_size(bytes, buffer, sizeof(buffer));
        printf("%4s ", buffer);
    } else {
        printf("%ju ", round_up_units(bytes, unit));
    }
}

static time_t
selected_time(const struct stat *st, const Options *opt)
{
    switch (opt->time_mode) {
    case TIME_ATIME: return st->st_atime;
    case TIME_CTIME: return st->st_ctime;
    case TIME_MTIME:
    default: return st->st_mtime;
    }
}

static void
format_time(const struct stat *st, const Options *opt,
    char *buffer, size_t buffer_size)
{
    time_t timestamp = selected_time(st, opt);
    time_t now = time(NULL);
    struct tm tm_value;
    const char *format;

    /* Match the common ls convention: old/future files show a year. */
    if (difftime(now, timestamp) > 180.0 * 24.0 * 60.0 * 60.0 ||
        difftime(timestamp, now) > 60.0 * 60.0)
        format = "%b %e  %Y";
    else
        format = "%b %e %H:%M";

    if (localtime_r(&timestamp, &tm_value) == NULL ||
        strftime(buffer, buffer_size, format, &tm_value) == 0)
        (void)snprintf(buffer, buffer_size, "unknown");
}

static void
format_file_size(const struct stat *st, const Options *opt,
    char *buffer, size_t buffer_size)
{
    if (opt->long_human) {
        human_size(st->st_size < 0 ? 0 : (uintmax_t)st->st_size,
            buffer, buffer_size);
        return;
    }

#if defined(major) && defined(minor)
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
        (void)snprintf(buffer, buffer_size, "%ju,%ju",
            (uintmax_t)major(st->st_rdev),
            (uintmax_t)minor(st->st_rdev));
        return;
    }
#endif

    (void)snprintf(buffer, buffer_size, "%jd", (intmax_t)st->st_size);
}

static void
format_owner_group(const struct stat *st, const Options *opt,
    char *owner, size_t owner_size, char *group, size_t group_size)
{
    if (opt->numeric) {
        (void)snprintf(owner, owner_size, "%ju", (uintmax_t)st->st_uid);
        (void)snprintf(group, group_size, "%ju", (uintmax_t)st->st_gid);
    } else {
        struct passwd *pw = getpwuid(st->st_uid);
        struct group *gr = getgrgid(st->st_gid);

        if (pw != NULL)
            (void)snprintf(owner, owner_size, "%s", pw->pw_name);
        else
            (void)snprintf(owner, owner_size, "%ju", (uintmax_t)st->st_uid);

        if (gr != NULL)
            (void)snprintf(group, group_size, "%s", gr->gr_name);
        else
            (void)snprintf(group, group_size, "%ju", (uintmax_t)st->st_gid);
    }
}

static void
print_long_entry(const char *name, const char *path,
    const struct stat *st, const Options *opt,
    int owner_width, int group_width, int size_width)
{
    char mode[11];
    char owner[128];
    char group[128];
    char size_buffer[64];
    char time_buffer[32];

    format_mode(st->st_mode, mode);
    format_owner_group(st, opt, owner, sizeof(owner), group, sizeof(group));
    format_file_size(st, opt, size_buffer, sizeof(size_buffer));
    format_time(st, opt, time_buffer, sizeof(time_buffer));

    /* Match ls column layout: two spaces before link count, two between
       owner/group, and a right-aligned size column (minimum width four). */
    printf("%s %2ju %-*s  %-*s %*s %s ",
        mode,
        (uintmax_t)st->st_nlink,
        owner_width, owner,
        group_width, group,
        size_width, size_buffer,
        time_buffer);
    print_name_suffix(name, st, opt);

    if (S_ISLNK(st->st_mode)) {
        char target[4096];
        ssize_t len = readlink(path, target, sizeof(target) - 1);
        if (len >= 0) {
            target[len] = '\0';
            fputs(" -> ", stdout);
            print_name(target, opt);
        }
    }

    putchar('\n');
}

static int
string_length_int(const char *text)
{
    size_t length = strlen(text);
    return length > (size_t)INT_MAX ? INT_MAX : (int)length;
}

static void
update_long_widths(const struct stat *st, const Options *opt,
    int *owner_width, int *group_width, int *size_width)
{
    char owner[128];
    char group[128];
    char size_buffer[64];
    int length;

    format_owner_group(st, opt, owner, sizeof(owner), group, sizeof(group));
    format_file_size(st, opt, size_buffer, sizeof(size_buffer));

    length = string_length_int(owner);
    if (length > *owner_width)
        *owner_width = length;

    length = string_length_int(group);
    if (length > *group_width)
        *group_width = length;

    length = string_length_int(size_buffer);
    if (length > *size_width)
        *size_width = length;
}

static void
print_entry(const FileEntry *entry, const Options *opt,
    int owner_width, int group_width, int size_width)
{
    if (opt->inode)
        printf("%ju ", (uintmax_t)entry->st.st_ino);

    print_block_prefix(&entry->st, opt);

    if (opt->long_format)
        print_long_entry(entry->name, entry->path, &entry->st, opt,
            owner_width, group_width, size_width);
    else {
        print_name_suffix(entry->name, &entry->st, opt);
        putchar('\n');
    }
}

void
display_list(const FileList *list, const Options *opt)
{
    size_t i;
    int owner_width = 1;
    int group_width = 1;
    int size_width = 4;
    int show_total = opt->long_format ||
        (opt->blocks && isatty(STDOUT_FILENO));

    if (opt->long_format) {
        for (i = 0; i < list->count; i++)
            update_long_widths(&list->items[i].st, opt,
                &owner_width, &group_width, &size_width);
    }

    if (show_total) {
        uintmax_t bytes = 0;
        uintmax_t unit;
        char buffer[64];

        for (i = 0; i < list->count; i++) {
            const struct stat *st = &list->items[i].st;
            if (opt->blocks && opt->size_mode == SIZE_HUMAN)
                bytes += st->st_size < 0 ? 0 : (uintmax_t)st->st_size;
            else
                bytes += allocated_bytes(st);
        }

        unit = total_block_unit(opt);
        if (unit == 0) {
            human_size(bytes, buffer, sizeof(buffer));
            printf("total %s\n", buffer);
        } else {
            printf("total %ju\n", round_up_units(bytes, unit));
        }
    }

    for (i = 0; i < list->count; i++)
        print_entry(&list->items[i], opt,
            owner_width, group_width, size_width);
}

void
display_operand(const char *name, const char *path,
    const struct stat *st, const Options *opt)
{
    FileEntry entry;
    int owner_width = 1;
    int group_width = 1;
    int size_width = 4;

    entry.name = (char *)name;
    entry.path = (char *)path;
    entry.st = *st;

    if (opt->long_format)
        update_long_widths(st, opt, &owner_width, &group_width, &size_width);

    print_entry(&entry, opt, owner_width, group_width, size_width);
}

void
display_header(const char *path, const Options *opt, int already_printed)
{
    if (already_printed)
        putchar('\n');
    print_name(path, opt);
    puts(":");
}
