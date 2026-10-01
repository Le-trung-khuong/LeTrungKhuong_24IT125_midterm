#ifndef SORT_H
#define SORT_H

#include <stddef.h>
#include "ls.h"
#include "options.h"

/* Sort entries according to -f -S -t -c -u -r. */
void sort_entries(FileEntry *entries, size_t count, const Options *options);

#endif
