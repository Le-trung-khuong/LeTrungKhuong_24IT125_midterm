#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include "options.h"

/* malloc/realloc/strdup that terminate the program on failure. */
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *text);

/* Join "dir" and "name" with exactly one '/' between them. */
char *join_path(const char *directory, const char *name);

/* Copy of `name` where non-printable characters become '?' (-q) or are kept (-w). */
char *printable_name(const char *name, const Options *options);

/* Write a size such as 1536 as "1.5K" into buf. */
void humanize(char *buf, size_t len, unsigned long long bytes);

/* Block size from $BLOCKSIZE (512 when unset or invalid). */
unsigned long blocksize_from_env(void);

#endif
