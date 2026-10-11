#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "filesystem.h"
#include "sort.h"
#include "display.h"

typedef struct {
    const char *path;
    struct stat lst;
    int is_directory;
} Operand;

typedef struct {
    int printed_section;
} ListingContext;

static int
list_directory_recursive(const char *path, const Options *opt,
    int show_header, ListingContext *context)
{
    FileList list;
    int read_result;
    int status = 0;
    size_t i;

    if (show_header) {
        display_header(path, opt, context->printed_section);
        context->printed_section = 1;
    }

    read_result = read_directory(path, opt, &list);
    if (read_result != 0 && list.count == 0) {
        free_filelist(&list);
        return 1;
    }
    if (read_result != 0)
        status = 1;

    sort_entries(&list, opt);
    display_list(&list, opt);
    /* A nested directory header still needs a blank line after this listing,
       even when the single top-level operand's header is suppressed. */
    context->printed_section = 1;

    if (opt->recursive) {
        for (i = 0; i < list.count; i++) {
            const FileEntry *entry = &list.items[i];

            if (strcmp(entry->name, ".") == 0 ||
                strcmp(entry->name, "..") == 0)
                continue;

            /* lstat metadata prevents recursion through symlinks. */
            if (S_ISDIR(entry->st.st_mode)) {
                if (list_directory_recursive(entry->path, opt, 1,
                        context) != 0)
                    status = 1;
            }
        }
    }

    free_filelist(&list);
    return status;
}

static int
compare_operand_paths(const Operand *a, const Operand *b, int reverse)
{
    int result = strcmp(a->path, b->path);
    if (result < 0) result = -1;
    else if (result > 0) result = 1;

    if (reverse)
        result = -result;
    return result;
}

static void
sort_operands(Operand **items, size_t count, int reverse)
{
    size_t i;

    /* Stable insertion sort is adequate for the usually small argv list. */
    for (i = 1; i < count; i++) {
        Operand *key = items[i];
        size_t j = i;

        while (j > 0 && compare_operand_paths(items[j - 1], key, reverse) > 0) {
            items[j] = items[j - 1];
            j--;
        }
        items[j] = key;
    }
}

int
main(int argc, char **argv)
{
    Options opt;
    int first_operand;
    int input_count;
    size_t path_count;
    size_t valid_count = 0;
    size_t file_count = 0;
    size_t dir_count = 0;
    size_t i;
    int status = 0;
    Operand *operands;
    Operand **files;
    Operand **dirs;
    ListingContext context;

    options_init(&opt);
    first_operand = options_parse(&opt, argc, argv);
    if (first_operand < 0)
        return 1;

    input_count = argc - first_operand;
    path_count = input_count == 0 ? 1U : (size_t)input_count;
    operands = calloc(path_count, sizeof(*operands));
    files = calloc(path_count, sizeof(*files));
    dirs = calloc(path_count, sizeof(*dirs));
    if (operands == NULL || files == NULL || dirs == NULL) {
        perror("ls: allocate operands");
        free(operands);
        free(files);
        free(dirs);
        return 1;
    }

    for (i = 0; i < path_count; i++) {
        const char *path = input_count == 0 ? "." : argv[first_operand + (int)i];
        struct stat target_stat;
        Operand *op = &operands[valid_count];

        if (lstat(path, &op->lst) == -1) {
            perror(path);
            status = 1;
            continue;
        }

        op->path = path;
        op->is_directory = 0;

        if (!opt.directories_as_files) {
            if (S_ISLNK(op->lst.st_mode)) {
                if (stat(path, &target_stat) == 0 && S_ISDIR(target_stat.st_mode))
                    op->is_directory = 1;
            } else if (S_ISDIR(op->lst.st_mode)) {
                op->is_directory = 1;
            }
        }

        if (op->is_directory)
            dirs[dir_count++] = op;
        else
            files[file_count++] = op;
        valid_count++;
    }

    if (!opt.force_unsorted) {
        sort_operands(files, file_count, opt.reverse);
        sort_operands(dirs, dir_count, opt.reverse);
    }

    for (i = 0; i < file_count; i++)
        display_operand(files[i]->path, files[i]->path,
            &files[i]->lst, &opt);

    context.printed_section = 0;
    for (i = 0; i < dir_count; i++) {
        /* NetBSD suppresses the root header for one explicit -R operand;
           headers are printed for nested directories by recursive calls. */
        int show_header = input_count > 1;
        if (list_directory_recursive(dirs[i]->path, &opt,
                show_header, &context) != 0)
            status = 1;
    }

    free(operands);
    free(files);
    free(dirs);
    return status;
}
