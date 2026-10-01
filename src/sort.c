/* sort.c - ordering of directory entries. */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sort.h"

/* qsort() comparators cannot receive extra arguments, so keep a pointer. */
static const Options *current_options;

/* Pick the time field chosen by -c / -u (default: modification time). */
static time_t entry_time(const FileEntry *entry)
{
    if (current_options->use_access) {
        return entry->st.st_atime;
    }
    if (current_options->use_status) {
        return entry->st.st_ctime;
    }
    return entry->st.st_mtime;
}

static int compare_entries(const void *left, const void *right)
{
    const FileEntry *a = left;
    const FileEntry *b = right;
    int result = 0;

    if (current_options->sort_size) {
        /* largest first */
        if (a->st.st_size < b->st.st_size) {
            result = 1;
        } else if (a->st.st_size > b->st.st_size) {
            result = -1;
        }
    } else if (current_options->sort_time) {
        /* newest first */
        time_t ta = entry_time(a);
        time_t tb = entry_time(b);

        if (ta < tb) {
            result = 1;
        } else if (ta > tb) {
            result = -1;
        }
    }

    /* equal keys (or no key): lexicographical order of the names */
    if (result == 0) {
        result = strcmp(a->name, b->name);
    }

    if (current_options->reverse) {
        result = -result;
    }
    return result;
}

void sort_entries(FileEntry *entries, size_t count, const Options *options)
{
    if (options->no_sort || count < 2) {
        return;
    }
    current_options = options;
    qsort(entries, count, sizeof(FileEntry), compare_entries);
}
