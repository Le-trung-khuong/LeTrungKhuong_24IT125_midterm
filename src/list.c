/* list.c - reading directories and driving the output. */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "columns.h"
#include "display.h"
#include "list.h"
#include "sort.h"
#include "util.h"

void free_entries(FileEntry *entries, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        free(entries[i].name);
        free(entries[i].path);
    }
    free(entries);
}

/* Decide whether a directory entry is shown (-a / -A rules). */
static int should_show(const char *name, const Options *options)
{
    if (options->all) {
        return 1;
    }
    if (name[0] != '.') {
        return 1;
    }
    if (options->almost_all) {
        return strcmp(name, ".") != 0 && strcmp(name, "..") != 0;
    }
    return 0;
}

/* Print sorted entries: columns on a terminal, one per line otherwise. */
static void print_all(const FileEntry *entries, size_t count,
                      const Options *options, const Widths *widths)
{
    size_t i;

    if (count > 0 && use_columns(options)) {
        print_columns(entries, count, options, widths);
        return;
    }
    for (i = 0; i < count; i++) {
        print_entry(&entries[i], options, widths);
    }
}

void list_entries(FileEntry *entries, size_t count, const Options *options)
{
    Widths widths;

    sort_entries(entries, count, options);
    compute_widths(entries, count, options, &widths);
    print_all(entries, count, options, &widths);
}

/* Read every visible entry of `path` into a freshly allocated array. */
static FileEntry *read_directory(const char *path, const Options *options,
                                 size_t *count_out, int *status)
{
    DIR *directory = opendir(path);
    struct dirent *item;
    FileEntry *entries = NULL;
    size_t count = 0;
    size_t capacity = 0;

    *count_out = 0;

    if (directory == NULL) {
        fprintf(stderr, "ls: %s: %s\n", path, strerror(errno));
        *status = 1;
        return NULL;
    }

    while ((item = readdir(directory)) != NULL) {
        char *full_path;

        if (!should_show(item->d_name, options)) {
            continue;
        }

        full_path = join_path(path, item->d_name);

        if (count == capacity) {
            capacity = capacity == 0 ? 32 : capacity * 2;
            entries = xrealloc(entries, capacity * sizeof(FileEntry));
        }

        /* lstat(): a symbolic link is shown as a link, not followed. */
        if (lstat(full_path, &entries[count].st) == -1) {
            fprintf(stderr, "ls: %s: %s\n", full_path, strerror(errno));
            free(full_path);
            *status = 1;
            continue;
        }

        entries[count].name = xstrdup(item->d_name);
        entries[count].path = full_path;
        count++;
    }

    closedir(directory);
    *count_out = count;
    return entries;
}

int list_directory(const char *path, const Options *options,
                   int show_header, int *printed)
{
    FileEntry *entries;
    size_t count;
    size_t i;
    int status = 0;

    /* blank line between two blocks of output */
    if (*printed) {
        putchar('\n');
    }
    if (show_header) {
        printf("%s:\n", path);
    }
    *printed = 1;

    entries = read_directory(path, options, &count, &status);
    if (entries == NULL && count == 0 && status != 0) {
        return status;          /* directory could not be opened */
    }

    sort_entries(entries, count, options);

    if (count > 0 && total_wanted(options)) {
        print_total(entries, count, options);
    }

    {
        Widths widths;

        compute_widths(entries, count, options, &widths);
        print_all(entries, count, options, &widths);
    }

    /* -R: descend into sub-directories, never into '.' or '..'. */
    if (options->recursive) {
        for (i = 0; i < count; i++) {
            if (S_ISDIR(entries[i].st.st_mode) &&
                strcmp(entries[i].name, ".") != 0 &&
                strcmp(entries[i].name, "..") != 0) {
                if (list_directory(entries[i].path, options, 1,
                                   printed) != 0) {
                    status = 1;
                }
            }
        }
    }

    free_entries(entries, count);
    return status;
}
