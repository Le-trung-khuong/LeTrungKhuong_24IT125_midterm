#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * Every command-line flag of ls(1) that this project supports.
 * Each member is 0 (off) or 1 (on), except `blocksize`.
 */
typedef struct {
    int all;            /* -a  include entries starting with '.'          */
    int almost_all;     /* -A  like -a but without '.' and '..'           */
    int classify;       /* -F  append / * @ % = | after names              */
    int long_format;    /* -l  long listing                                */
    int numeric_ids;    /* -n  like -l, owner/group shown as numbers       */
    int recursive;      /* -R  list subdirectories recursively             */
    int reverse;        /* -r  reverse the sort order                      */
    int sort_size;      /* -S  sort by size, largest first                 */
    int sort_time;      /* -t  sort by time, newest first                  */
    int use_access;     /* -u  use access time                             */
    int use_status;     /* -c  use status-change time                      */
    int inode;          /* -i  print inode number                          */
    int blocks;         /* -s  print number of blocks                      */
    int human;          /* -h  human readable sizes                        */
    int kilo;           /* -k  block counts in 1024-byte units             */
    int no_sort;        /* -f  do not sort                                 */
    int list_dir;       /* -d  list directories as plain files             */
    int question;       /* -q  non-printable characters shown as '?'       */
    int raw;            /* -w  non-printable characters printed raw        */
    unsigned long blocksize; /* unit used by -s and "total" (512 default)  */
} Options;

/* Put every option in its default state (depends on isatty()). */
void init_options(Options *options);

/*
 * Parse argv. Returns the index of the first operand (file name),
 * or -1 on an unknown option (usage message already printed).
 */
int parse_options(int argc, char **argv, Options *options);

#endif
