/* util.c - small helpers: safe allocation, paths, names, sizes. */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "util.h"

void *xmalloc(size_t size)
{
    void *ptr = malloc(size == 0 ? 1 : size);

    if (ptr == NULL) {
        fprintf(stderr, "ls: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size)
{
    void *result = realloc(ptr, size == 0 ? 1 : size);

    if (result == NULL) {
        fprintf(stderr, "ls: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return result;
}

char *xstrdup(const char *text)
{
    size_t length = strlen(text);
    char *copy = xmalloc(length + 1);

    memcpy(copy, text, length + 1);
    return copy;
}

char *join_path(const char *directory, const char *name)
{
    size_t dir_len = strlen(directory);
    size_t name_len = strlen(name);
    int need_slash = dir_len > 0 && directory[dir_len - 1] != '/';
    char *result = xmalloc(dir_len + need_slash + name_len + 1);

    memcpy(result, directory, dir_len);
    if (need_slash) {
        result[dir_len] = '/';
    }
    memcpy(result + dir_len + need_slash, name, name_len + 1);
    return result;
}

char *printable_name(const char *name, const Options *options)
{
    size_t length = strlen(name);
    size_t in = 0;
    size_t out = 0;
    char *result = xmalloc(length + 1);
    mbstate_t state;

    /* -w (raw): copy the name unchanged. */
    if (options->raw) {
        memcpy(result, name, length + 1);
        return result;
    }

    /*
     * -q: walk through the name one (multibyte) character at a time so
     * that valid UTF-8 names such as Vietnamese ones stay readable and
     * only really non-printable characters become '?'.
     */
    memset(&state, 0, sizeof(state));
    while (in < length) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, name + in, length - in, &state);

        if (n == (size_t)-1 || n == (size_t)-2) {
            /* invalid or truncated sequence: replace one byte */
            result[out++] = '?';
            in++;
            memset(&state, 0, sizeof(state));
        } else if (iswprint((wint_t)wc)) {
            memcpy(result + out, name + in, n);
            out += n;
            in += n;
        } else {
            result[out++] = '?';
            in += (n == 0) ? 1 : n;
        }
    }
    result[out] = '\0';
    return result;
}

void humanize(char *buf, size_t len, unsigned long long bytes)
{
    const char units[] = "KMGTPE";
    double value;
    int unit = -1;

    /* below 1 KiB: plain bytes with a "B" suffix, like NetBSD ls -h */
    if (bytes < 1024ULL) {
        snprintf(buf, len, "%lluB", bytes);
        return;
    }

    /* divide by 1024 until the value is smaller than 1024 */
    value = (double)bytes;
    while (value >= 1024.0 && unit < 5) {
        value /= 1024.0;
        unit++;
    }

    /* one decimal below 10 (e.g. 2.9K), no decimals from 10 up (e.g. 15M) */
    if (value < 9.95) {
        snprintf(buf, len, "%.1f%c", value, units[unit]);
    } else {
        snprintf(buf, len, "%.0f%c", value, units[unit]);
    }
}

unsigned long blocksize_from_env(void)
{
    const char *text = getenv("BLOCKSIZE");
    char *end;
    unsigned long number;

    if (text == NULL || *text == '\0') {
        return 512UL;
    }

    number = strtoul(text, &end, 10);
    if (end == text) {
        number = 1;              /* "K" means 1K */
    }

    switch (*end) {
    case '\0':
        break;
    case 'k': case 'K':
        number *= 1024UL;
        break;
    case 'm': case 'M':
        number *= 1024UL * 1024UL;
        break;
    case 'g': case 'G':
        number *= 1024UL * 1024UL * 1024UL;
        break;
    default:
        return 512UL;
    }

    return number < 512UL ? 512UL : number;
}
