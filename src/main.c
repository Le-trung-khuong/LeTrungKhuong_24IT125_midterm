/* main.c - ls(1) clone: argument handling and operand processing. */

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "ls.h"
#include "list.h"
#include "options.h"
#include "sort.h"
#include "util.h"

/*
 * Examine one operand. On success the entry is appended to `files`
 * (non-directories) or `dirs` (directories). Returns 0 or 1 (error).
 */
static int classify_operand(const char *operand, const Options *options,
                            FileEntry **files, size_t *file_count,
                            FileEntry **dirs, size_t *dir_count)
{
    struct stat st;
    FileEntry entry;
    FileEntry **list;
    size_t *count;
    int follow;

    /*
     * -d, -l (-n) and -F show a symbolic link itself.
     * Otherwise a link to a directory is followed (POSIX behaviour).
     */
    follow = !options->list_dir && !options->long_format &&
             !options->classify;

    if (follow) {
        if (stat(operand, &st) == -1 && lstat(operand, &st) == -1) {
            fprintf(stderr, "ls: %s: %s\n", operand, strerror(errno));
            return 1;
        }
    } else if (lstat(operand, &st) == -1) {
        fprintf(stderr, "ls: %s: %s\n", operand, strerror(errno));
        return 1;
    }

    entry.name = xstrdup(operand);
    entry.path = xstrdup(operand);
    entry.st = st;

    if (S_ISDIR(st.st_mode) && !options->list_dir) {
        list = dirs;
        count = dir_count;
    } else {
        list = files;
        count = file_count;
    }

    /* arrays were allocated with room for every operand */
    (*list)[(*count)++] = entry;
    return 0;
}

int main(int argc, char **argv)
{
    Options options;
    FileEntry *files;
    FileEntry *dirs;
    size_t file_count = 0;
    size_t dir_count = 0;
    char *default_operand[1] = { "." };
    char **operands;
    int operand_count;
    int first_operand;
    int status = 0;
    int printed = 0;
    int show_header;
    int i;

    /* Only the character classification follows the user's locale. */
    setlocale(LC_CTYPE, "");

    init_options(&options);
    first_operand = parse_options(argc, argv, &options);
    if (first_operand < 0) {
        return EXIT_FAILURE;
    }

    operand_count = argc - first_operand;
    if (operand_count == 0) {
        operands = default_operand;
        operand_count = 1;
    } else {
        operands = &argv[first_operand];
    }

    files = xmalloc((size_t)operand_count * sizeof(FileEntry));
    dirs = xmalloc((size_t)operand_count * sizeof(FileEntry));

    for (i = 0; i < operand_count; i++) {
        if (classify_operand(operands[i], &options, &files, &file_count,
                             &dirs, &dir_count) != 0) {
            status = 1;
        }
    }

    /* Non-directories first, then directories; each group sorted alone. */
    if (file_count > 0) {
        list_entries(files, file_count, &options);
        printed = 1;
    }

    sort_entries(dirs, dir_count, &options);

    /*
     * "dir:" header for the operands only when there are several of them
     * (NetBSD behaviour). With -R, sub-directories always get a header.
     */
    show_header = operand_count > 1;

    for (i = 0; i < (int)dir_count; i++) {
        if (list_directory(dirs[i].path, &options, show_header,
                           &printed) != 0) {
            status = 1;
        }
    }

    free_entries(files, file_count);
    free_entries(dirs, dir_count);

    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}
