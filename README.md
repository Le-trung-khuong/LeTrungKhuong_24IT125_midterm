# Midterm  Implement ls(1)Project 

## Student Information

- Full name: Le Trung Khuong
- Student ID: 24IT125
- Class: 24JIT
- GitHub repository: https://github.com/Le-trung-khuong/LeTrungKhuong_24IT125_midterm

## 1. Introduction

This project is a simplified clone of the UNIX `ls(1)` command written in C.
It follows the NetBSD `ls(1)` manual page given with the assignment
(`ls [-AacdFfhiklnqRrSstuw] [file ...]`) and uses only system calls and
standard library functions (`opendir`, `readdir`, `lstat`, `stat`,
`readlink`, `getpwuid`, `getgrgid`, ...).

## 2. Build and Run

```bash
make            # build ./myls
make debug      # build with -g, AddressSanitizer and UBSan
make test       # compare ./myls with the system ls
make clean      # remove binaries and object files
```

```bash
./myls [OPTIONS] [FILE ...]
```

With no operand the current directory is listed.

## 3. Supported Options

| Option | Behaviour |
|---|---|
| `-A` | All entries except `.` and `..` (always set for root) |
| `-a` | Include entries starting with `.` |
| `-c` | Use status-change time for `-t` and `-l` |
| `-d` | List directories as plain files; symlink operands are not followed |
| `-F` | Append `/` dir, `*` executable, `@` symlink, `%` whiteout, `=` socket, `\|` FIFO |
| `-f` | Do not sort (also lists dot files, like NetBSD `ls`) |
| `-h` | Human readable sizes for `-s` and `-l` (overrides `-k`) |
| `-i` | Print inode number |
| `-k` | `-s` block counts in 1024-byte units (rightmost of `-k`/`-h` wins) |
| `-l` | Long format |
| `-n` | Like `-l` with numeric owner and group (`-l`/`-n`: last one wins) |
| `-q` | Print non-printable characters as `?` (default on a terminal) |
| `-R` | Recursive listing (`-R`/`-d`: last one wins) |
| `-r` | Reverse sort order |
| `-S` | Sort by size, largest first |
| `-s` | Print number of 512-byte (or `BLOCKSIZE`) blocks; `total` line on a terminal |
| `-t` | Sort by time, newest first |
| `-u` | Use access time for `-t` and `-l` (`-c`/`-u`: last one wins) |
| `-w` | Print non-printable characters raw (default when not a terminal) |

## 4. Project Structure

```text
include/  ls.h  options.h  list.h  display.h  sort.h  util.h
src/      main.c options.c list.c display.c sort.c util.c
tests/    run_tests.sh
Makefile  README.md  .gitignore
```

| Module | Responsibility |
|---|---|
| `main.c` | Parses operands, separates files from directories, prints files first, then directories |
| `options.c` | `getopt()` parsing; "last option wins" rules; `-A` for root |
| `list.c` | `opendir/readdir/lstat`, filtering for `-a/-A`, recursion for `-R` |
| `sort.c` | `qsort()` comparator: name, `-S`, `-t` with `-c/-u`, `-r`, `-f` |
| `display.c` | Mode string, owner/group, size, time, `-F` character, link target, column widths |
| `util.c` | Checked allocation, path joining, `-q/-w` names, `-h` sizes, `BLOCKSIZE` |

## 5. Implementation Notes

- **Directory reading:** `opendir()`, `readdir()`, `closedir()`.
- **Metadata:** `lstat()` so symbolic links are shown as links. Operands that
  are links to directories are followed with `stat()` unless `-d`, `-l`, `-n`
  or `-F` is used.
- **Operands:** non-directory operands are printed first, then directories;
  the two groups are sorted separately. Headers (`dir:`) are printed when
  there is more than one operand or with `-R`.
- **Long format:** mode, link count, owner, group, size (or `major, minor`
  for devices), date, name, and ` -> target` for symbolic links. Dates older
  than about six months (or in the future) show the year instead of the time,
  like the system `ls`. A `total` line (in 512-byte blocks) precedes each
  directory.
- **Robustness:** every allocation is checked, every system call error is
  reported as `ls: path: reason` and the exit status becomes 1. Empty
  directories, missing files, broken symlinks and permission errors do not
  crash the program.
- **Recursion:** `-R` never follows symbolic links to directories and skips
  `.` and `..`, so there are no infinite loops.

## 6. Testing

`make test` builds a fixed directory (regular files, hidden file, executable,
symlink, broken symlink, symlink to directory, FIFO, empty directory,
sub-directory) and compares `./myls` output with the system `ls` for many
option combinations, then checks that error cases do not crash, that the exit
status is non-zero on errors, and that `-q`/`-w` behave correctly.

Manual commands:

```bash
./myls -l testdir
./myls -lh testdir
./myls -laS testdir
./myls -ltr testdir
./myls -R testdir
./myls -F testdir
./myls -d testdir
./myls -i -s testdir
./myls file1 dir1 missing     # files first, then directories, error for missing
```

## 7. Limitations

- Output is one entry per line (the default in the manual); no multi-column mode.
- Archive states `a`/`A` in the mode field are not implemented.
- Differences from GNU `ls` that follow NetBSD `ls`: `-A` is implied for
  root, `-f` implies `-a`, `-h` prints `0B`/`7B` for small sizes, the long
  format uses two spaces between columns, `-R` with a single operand prints no
  header for that operand (only for sub-directories), and `-s` prints `total`
  only when output is a terminal.
- Sorting compares whole seconds only (no nanosecond tie-break).

## 8. Conclusion

The project covers every option in the assignment manual, is split into
several modules with headers, builds with `make`, and was tested against the
system `ls` and with AddressSanitizer.
