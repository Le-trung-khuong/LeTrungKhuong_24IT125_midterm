#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include "ls.h"
#include "options.h"

/* Print already-collected entries (used for non-directory operands). */
void list_entries(FileEntry *entries, size_t count, const Options *options);

/*
 * List the contents of one directory (and subdirectories with -R).
 *   show_header  print "path:" before the listing
 *   printed      set to 1 after output; used to put blank lines between blocks
 * Returns 0 on success, 1 if any error was reported.
 */
int list_directory(const char *path, const Options *options,
                   int show_header, int *printed);

#endif
