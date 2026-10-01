#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>
#include "ls.h"
#include "options.h"

/* Column widths so that numbers line up in -i, -s and -l output. */
typedef struct {
    int inode;
    int blocks;
    int nlink;
    int owner;
    int group;
    int size;
} Widths;

void compute_widths(const FileEntry *entries, size_t count,
                    const Options *options, Widths *widths);
void print_entry(const FileEntry *entry, const Options *options,
                 const Widths *widths);

/* Is the "total N" line required for a directory listing? */
int total_wanted(const Options *options);
void print_total(const FileEntry *entries, size_t count,
                 const Options *options);

#endif
