#ifndef COLUMNS_H
#define COLUMNS_H

#include <stddef.h>
#include "display.h"
#include "ls.h"
#include "options.h"

/* Width of the terminal in characters: ioctl(TIOCGWINSZ), $COLUMNS, or 80. */
int terminal_width(void);

/*
 * Print entries in several columns, filled top to bottom like ls(1) does on
 * a terminal. Falls back to one entry per line when nothing fits.
 */
void print_columns(const FileEntry *entries, size_t count,
                   const Options *options, const Widths *widths);

/* Does this option set use the column layout? (-l and -n always win.) */
int use_columns(const Options *options);

#endif
