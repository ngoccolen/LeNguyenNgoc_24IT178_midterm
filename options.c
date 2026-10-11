#include <stdio.h>
#include <unistd.h>

#include "options.h"

void
options_init(Options *opt)
{
    opt->all = 0;
    opt->almost_all = 0;
    opt->directories_as_files = 0;
    opt->classify = 0;
    opt->force_unsorted = 0;
    opt->inode = 0;
    opt->long_format = 0;
    opt->numeric = 0;
    opt->long_human = 0;
    opt->recursive = 0;
    opt->reverse = 0;
    opt->blocks = 0;

    opt->sort_mode = SORT_NAME;
    opt->time_mode = TIME_MTIME;
    opt->size_mode = SIZE_DEFAULT;
    opt->name_mode = NAME_AUTO;
}

int
options_parse(Options *opt, int argc, char **argv)
{
    int ch;

    opterr = 0;
    optind = 1;

    while ((ch = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (ch) {
        case 'A':
            opt->almost_all = 1;
            opt->all = 0;
            break;
        case 'a':
            opt->all = 1;
            opt->almost_all = 0;
            break;
        case 'c':
            opt->time_mode = TIME_CTIME;
            break;
        case 'd':
            opt->directories_as_files = 1;
            opt->recursive = 0;
            break;
        case 'F':
            opt->classify = 1;
            break;
        case 'f':
            /* NetBSD -f disables sorting and includes all directory entries. */
            opt->force_unsorted = 1;
            opt->sort_mode = SORT_NONE;
            opt->all = 1;
            opt->almost_all = 0;
            break;
        case 'h':
            opt->long_human = 1;
            opt->size_mode = SIZE_HUMAN;
            break;
        case 'i':
            opt->inode = 1;
            break;
        case 'k':
            /* The last of -h and -k wins. -k disables human sizes in -l. */
            opt->size_mode = SIZE_KB;
            opt->long_human = 0;
            break;
        case 'l':
            opt->long_format = 1;
            opt->numeric = 0;
            break;
        case 'n':
            opt->long_format = 1;
            opt->numeric = 1;
            break;
        case 'q':
            opt->name_mode = NAME_QUESTION;
            break;
        case 'R':
            opt->recursive = 1;
            opt->directories_as_files = 0;
            break;
        case 'r':
            opt->reverse = 1;
            break;
        case 'S':
            opt->sort_mode = SORT_SIZE;
            break;
        case 's':
            opt->blocks = 1;
            break;
        case 't':
            opt->sort_mode = SORT_TIME;
            break;
        case 'u':
            opt->time_mode = TIME_ATIME;
            break;
        case 'w':
            opt->name_mode = NAME_RAW;
            break;
        default:
            fprintf(stderr, "ls: invalid option -- %c\n", optopt ? optopt : ch);
            fprintf(stderr, "usage: ls [-AacdFfhiklnqRrSstuw] [file ...]\n");
            return -1;
        }
    }

    /* NetBSD ls shows almost-all entries by default for the super-user. */
    if (geteuid() == 0 && !opt->all) {
        opt->almost_all = 1;
    }

    return optind;
}
