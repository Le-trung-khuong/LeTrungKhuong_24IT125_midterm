/* columns.c - multi-column output used when stdout is a terminal (or -C). */

#ifdef __linux__
#define _XOPEN_SOURCE 700       /* wcswidth() in glibc */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wchar.h>

#include "columns.h"
#include "util.h"

#define COLUMN_GAP 2

int use_columns(const Options *options)
{
    return options->columns && !options->long_format;
}

int terminal_width(void)
{
    struct winsize ws;
    const char *env;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return (int)ws.ws_col;
    }
    env = getenv("COLUMNS");
    if (env != NULL && atoi(env) > 0) {
        return atoi(env);
    }
    return 80;
}

/* Number of screen cells the text occupies (UTF-8 aware). */
static int display_width(const char *text)
{
    size_t length = strlen(text);
    wchar_t *wide = xmalloc((length + 1) * sizeof(wchar_t));
    int result;

    if (mbstowcs(wide, text, length + 1) == (size_t)-1) {
        result = (int)length;           /* not valid in this locale */
    } else {
        result = wcswidth(wide, length + 1);
        if (result < 0) {
            result = (int)length;
        }
    }
    free(wide);
    return result;
}

/* Does a layout with `cols` columns fit in `limit` cells? Fills col_w. */
static int layout_fits(const int *width, size_t count, size_t cols,
                       size_t rows, int limit, int *col_w)
{
    size_t c, r;
    long total = 0;

    for (c = 0; c < cols; c++) {
        col_w[c] = 0;
        for (r = 0; r < rows; r++) {
            size_t index = c * rows + r;

            if (index < count && width[index] > col_w[c]) {
                col_w[c] = width[index];
            }
        }
        total += col_w[c] + (c + 1 < cols ? COLUMN_GAP : 0);
    }
    return total <= limit;
}

void print_columns(const FileEntry *entries, size_t count,
                   const Options *options, const Widths *widths)
{
    char **plain = xmalloc(count * sizeof(char *));
    int *width = xmalloc(count * sizeof(int));
    int *col_w = xmalloc(count * sizeof(int));
    int limit = terminal_width();
    size_t cols, rows = count, r, c, i;

    for (i = 0; i < count; i++) {
        plain[i] = format_short(&entries[i], options, widths, 0);
        width[i] = display_width(plain[i]);
    }

    /* The most columns that still fit; at least one. */
    for (cols = count; cols > 1; cols--) {
        rows = (count + cols - 1) / cols;
        /* skip layouts that would leave a column empty */
        if ((cols - 1) * rows >= count) {
            continue;
        }
        if (layout_fits(width, count, cols, rows, limit, col_w)) {
            break;
        }
    }
    if (cols <= 1) {
        cols = 1;
        rows = count;
        layout_fits(width, count, cols, rows, limit, col_w);
    }

    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            size_t index = c * rows + r;
            char *text;
            int pad;

            if (index >= count) {
                break;
            }
            text = format_short(&entries[index], options, widths, 1);
            fputs(text, stdout);
            free(text);

            /* no trailing spaces after the last item of a row */
            if (c + 1 < cols && (c + 1) * rows + r < count) {
                for (pad = col_w[c] - width[index] + COLUMN_GAP; pad > 0;
                     pad--) {
                    putchar(' ');
                }
            }
        }
        putchar('\n');
    }

    for (i = 0; i < count; i++) {
        free(plain[i]);
    }
    free(plain);
    free(width);
    free(col_w);
}
