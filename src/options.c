/* options.c - command-line parsing for ls(1). */

#include <stdio.h>
#include <unistd.h>

#include "options.h"
#include "util.h"

void init_options(Options *options)
{
    *options = (Options){0};

    /*
     * Manual: -q is the default when output is a terminal,
     * -w is the default when output is not a terminal.
     */
    options->question = isatty(STDOUT_FILENO) ? 1 : 0;
    options->raw = !options->question;
    options->blocksize = 512UL;
    options->columns = isatty(STDOUT_FILENO) ? 1 : 0;   /* like real ls */
}

int parse_options(int argc, char **argv, Options *options)
{
    int opt;

    while ((opt = getopt(argc, argv, "1ACacdFfGhiklnqRrSstuw")) != -1) {
        switch (opt) {
        case 'A': options->almost_all = 1; break;

        /* Extensions (not in the manual): -1 one per line, -C columns, -G colour. */
        case '1': options->columns = 0; break;
        case 'C': options->columns = 1; break;
        case 'G': options->color = 1; break;

        case 'a': options->all = 1; break;
        case 'F': options->classify = 1; break;
        case 'f': options->no_sort = 1; options->all = 1; break;
        case 'i': options->inode = 1; break;
        case 'r': options->reverse = 1; break;
        case 's': options->blocks = 1; break;

        /* -c and -u override each other: the last one wins. */
        case 'c': options->use_status = 1; options->use_access = 0; break;
        case 'u': options->use_access = 1; options->use_status = 0; break;

        /* -R and -d override each other. */
        case 'R': options->recursive = 1; options->list_dir = 0; break;
        case 'd': options->list_dir = 1; options->recursive = 0; break;

        /* -l and -n override each other; -n is -l with numeric ids. */
        case 'l': options->long_format = 1; options->numeric_ids = 0; break;
        case 'n': options->long_format = 1; options->numeric_ids = 1; break;

        /* -q and -w override each other. */
        case 'q': options->question = 1; options->raw = 0; break;
        case 'w': options->raw = 1; options->question = 0; break;

        /* -h and -k: the rightmost one wins. */
        case 'h': options->human = 1; options->kilo = 0; break;
        case 'k': options->kilo = 1; options->human = 0; break;

        /* Sorting keys: the last of -S / -t is used. */
        case 'S': options->sort_size = 1; options->sort_time = 0; break;
        case 't': options->sort_time = 1; options->sort_size = 0; break;

        default:
            fprintf(stderr, "usage: %s [-1ACacdFfGhiklnqRrSstuw] [file ...]\n",
                    argv[0]);
            return -1;
        }
    }

    /* -A is always set for the super-user. */
    if (geteuid() == 0) {
        options->almost_all = 1;
    }

    /* Unit for -s and the "total" line. */
    if (options->kilo) {
        options->blocksize = 1024UL;
    } else {
        options->blocksize = blocksize_from_env();
    }

    return optind;
}
