/* display.c - formatting and printing of one directory entry. */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/sysmacros.h>
#endif
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "display.h"
#include "util.h"

#define FIELD 64

/* Seconds in about six months: older files show the year, not the time. */
#define SIX_MONTHS 15778476L

/* ---------- small formatting helpers ---------- */

static void fmt_inode(char *buf, size_t len, const struct stat *st)
{
    snprintf(buf, len, "%llu", (unsigned long long)st->st_ino);
}

/*
 * Block count for -s and "total". Normally the number of 512-byte blocks is
 * converted to the block size (partial units rounded up). With -h the manual
 * says sizes are reported in bytes, so the file size is humanized instead.
 */
static void fmt_blocks(char *buf, size_t len, unsigned long long blocks512,
                       unsigned long long size, const Options *options)
{
    if (options->human) {
        humanize(buf, len, size);
    } else {
        unsigned long long unit = options->blocksize;
        unsigned long long bytes = blocks512 * 512ULL;

        snprintf(buf, len, "%llu", (bytes + unit - 1) / unit);
    }
}

static void fmt_nlink(char *buf, size_t len, const struct stat *st)
{
    snprintf(buf, len, "%lu", (unsigned long)st->st_nlink);
}

static void fmt_owner(char *buf, size_t len, const struct stat *st,
                      const Options *options)
{
    struct passwd *pw = options->numeric_ids ? NULL : getpwuid(st->st_uid);

    if (pw != NULL) {
        snprintf(buf, len, "%s", pw->pw_name);
    } else {
        snprintf(buf, len, "%lu", (unsigned long)st->st_uid);
    }
}

static void fmt_group(char *buf, size_t len, const struct stat *st,
                      const Options *options)
{
    struct group *gr = options->numeric_ids ? NULL : getgrgid(st->st_gid);

    if (gr != NULL) {
        snprintf(buf, len, "%s", gr->gr_name);
    } else {
        snprintf(buf, len, "%lu", (unsigned long)st->st_gid);
    }
}

/* Size column: bytes, human readable, or "major, minor" for devices. */
static void fmt_size(char *buf, size_t len, const struct stat *st,
                     const Options *options)
{
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
        snprintf(buf, len, "%lu, %lu",
                 (unsigned long)major(st->st_rdev),
                 (unsigned long)minor(st->st_rdev));
    } else if (options->human) {
        humanize(buf, len, (unsigned long long)st->st_size);
    } else {
        snprintf(buf, len, "%llu", (unsigned long long)st->st_size);
    }
}

/* Build the 10-character mode string, e.g. "drwxr-xr-x". buf needs 11 bytes. */
static void fmt_mode(char *buf, mode_t mode)
{
    char type = '?';

    if (S_ISREG(mode))       type = '-';
    else if (S_ISDIR(mode))  type = 'd';
    else if (S_ISLNK(mode))  type = 'l';
    else if (S_ISCHR(mode))  type = 'c';
    else if (S_ISBLK(mode))  type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';
#ifdef S_ISWHT
    else if (S_ISWHT(mode))  type = 'w';
#endif

    buf[0] = type;
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_ISUID) ? ((mode & S_IXUSR) ? 's' : 'S')
                              : ((mode & S_IXUSR) ? 'x' : '-');
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_ISGID) ? ((mode & S_IXGRP) ? 's' : 'S')
                              : ((mode & S_IXGRP) ? 'x' : '-');
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_ISVTX) ? ((mode & S_IXOTH) ? 't' : 'T')
                              : ((mode & S_IXOTH) ? 'x' : '-');
    buf[10] = '\0';
}

/* Time shown by -l: -u access, -c status change, otherwise modification. */
static time_t chosen_time(const struct stat *st, const Options *options)
{
    if (options->use_access) {
        return st->st_atime;
    }
    if (options->use_status) {
        return st->st_ctime;
    }
    return st->st_mtime;
}

static void fmt_time(char *buf, size_t len, time_t value)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&value);
    const char *format;

    if (tm_info == NULL) {
        snprintf(buf, len, "?");
        return;
    }

    /* Old or future files: month day  year. Recent files: month day hh:mm. */
    if (value > now || now - value > SIX_MONTHS) {
        format = "%b %e  %Y";
    } else {
        format = "%b %e %H:%M";
    }
    if (strftime(buf, len, format, tm_info) == 0) {
        snprintf(buf, len, "?");
    }
}

/* Character appended by -F, or 0 if none. */
static char classify_char(mode_t mode)
{
    if (S_ISDIR(mode))  return '/';
    if (S_ISLNK(mode))  return '@';
    if (S_ISSOCK(mode)) return '=';
    if (S_ISFIFO(mode)) return '|';
#ifdef S_ISWHT
    if (S_ISWHT(mode))  return '%';
#endif
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        return '*';
    }
    return '\0';
}

/* Print " -> target" for a symbolic link (long format only). */
static void print_link_target(const char *path, const Options *options)
{
    char target[4096];
    ssize_t length = readlink(path, target, sizeof(target) - 1);

    if (length >= 0) {
        char *shown;

        target[length] = '\0';
        shown = printable_name(target, options);
        printf(" -> %s", shown);
        free(shown);
    }
}

/* ---------- public functions ---------- */

static int wider(int current, const char *text)
{
    int length = (int)strlen(text);

    return length > current ? length : current;
}

void compute_widths(const FileEntry *entries, size_t count,
                    const Options *options, Widths *widths)
{
    size_t i;
    char buf[FIELD];

    memset(widths, 0, sizeof(*widths));

    for (i = 0; i < count; i++) {
        const struct stat *st = &entries[i].st;

        if (options->inode) {
            fmt_inode(buf, sizeof(buf), st);
            widths->inode = wider(widths->inode, buf);
        }
        if (options->blocks) {
            fmt_blocks(buf, sizeof(buf), (unsigned long long)st->st_blocks,
                       (unsigned long long)st->st_size, options);
            widths->blocks = wider(widths->blocks, buf);
        }
        if (options->long_format) {
            fmt_nlink(buf, sizeof(buf), st);
            widths->nlink = wider(widths->nlink, buf);
            fmt_owner(buf, sizeof(buf), st, options);
            widths->owner = wider(widths->owner, buf);
            fmt_group(buf, sizeof(buf), st, options);
            widths->group = wider(widths->group, buf);
            fmt_size(buf, sizeof(buf), st, options);
            widths->size = wider(widths->size, buf);
        }
    }
}

void print_entry(const FileEntry *entry, const Options *options,
                 const Widths *widths)
{
    const struct stat *st = &entry->st;
    char buf[FIELD];
    char name_buf[256];
    char *name;
    char suffix;

    if (options->inode) {
        fmt_inode(buf, sizeof(buf), st);
        printf("%*s ", widths->inode, buf);
    }

    if (options->blocks) {
        fmt_blocks(buf, sizeof(buf), (unsigned long long)st->st_blocks,
                   (unsigned long long)st->st_size, options);
        printf("%*s ", widths->blocks, buf);
    }

    if (options->long_format) {
        char mode[11];
        char owner[FIELD];
        char group[FIELD];

        fmt_mode(mode, st->st_mode);
        fmt_owner(owner, sizeof(owner), st, options);
        fmt_group(group, sizeof(group), st, options);

        fmt_nlink(buf, sizeof(buf), st);
        printf("%s  %*s ", mode, widths->nlink, buf);
        printf("%-*s  %-*s  ", widths->owner, owner, widths->group, group);

        fmt_size(buf, sizeof(buf), st, options);
        printf("%*s ", widths->size, buf);

        fmt_time(name_buf, sizeof(name_buf), chosen_time(st, options));
        printf("%s ", name_buf);
    }

    name = printable_name(entry->name, options);
    fputs(name, stdout);
    free(name);

    if (options->classify) {
        suffix = classify_char(st->st_mode);
        if (suffix != '\0') {
            putchar(suffix);
        }
    }

    if (options->long_format && S_ISLNK(st->st_mode)) {
        print_link_target(entry->path, options);
    }

    putchar('\n');
}

int total_wanted(const Options *options)
{
    /* -l always prints it; -s only when the output is a terminal. */
    return options->long_format ||
           (options->blocks && isatty(STDOUT_FILENO));
}

void print_total(const FileEntry *entries, size_t count, const Options *options)
{
    unsigned long long sum_blocks = 0;
    unsigned long long sum_size = 0;
    char buf[FIELD];
    size_t i;

    for (i = 0; i < count; i++) {
        sum_blocks += (unsigned long long)entries[i].st.st_blocks;
        sum_size += (unsigned long long)entries[i].st.st_size;
    }
    fmt_blocks(buf, sizeof(buf), sum_blocks, sum_size, options);
    printf("total %s\n", buf);
}
