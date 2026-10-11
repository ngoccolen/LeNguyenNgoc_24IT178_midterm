#ifndef OPTIONS_H
#define OPTIONS_H

typedef enum {
    SORT_NAME,
    SORT_TIME,
    SORT_SIZE,
    SORT_NONE
} SortMode;

typedef enum {
    TIME_MTIME,
    TIME_CTIME,
    TIME_ATIME
} TimeMode;

typedef enum {
    SIZE_DEFAULT,
    SIZE_KB,
    SIZE_HUMAN
} SizeMode;

typedef enum {
    NAME_AUTO,
    NAME_QUESTION,
    NAME_RAW
} NameMode;

typedef struct {
    int all;                    /* -a: include . and .. */
    int almost_all;             /* -A: include hidden, except . and .. */
    int directories_as_files;   /* -d */
    int classify;               /* -F */
    int force_unsorted;         /* -f */
    int inode;                  /* -i */
    int long_format;            /* -l or -n */
    int numeric;                /* -n */
    int long_human;             /* -h for -l */
    int recursive;              /* -R */
    int reverse;                /* -r */
    int blocks;                 /* -s */

    SortMode sort_mode;
    TimeMode time_mode;
    SizeMode size_mode;         /* block-count display: default, KB, human */
    NameMode name_mode;         /* automatic, quote nonprintable, raw */
} Options;

void options_init(Options *opt);
int options_parse(Options *opt, int argc, char **argv);

#endif
