#ifndef LS_H
#define LS_H

#include <stddef.h>
#include <sys/stat.h>

/* One file to be listed: its display name, full path and metadata. */
typedef struct {
    char *name;         /* name shown to the user                  */
    char *path;         /* path used for lstat()/readlink()        */
    struct stat st;     /* metadata (lstat, or stat for operands)  */
} FileEntry;

/* Release every string in the array, then the array itself. */
void free_entries(FileEntry *entries, size_t count);

#endif
