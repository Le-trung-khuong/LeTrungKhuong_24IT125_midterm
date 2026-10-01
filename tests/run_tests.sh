#!/bin/sh
# Compare ./myls with the system ls on a fixed test directory.
# Usage: make test      (or: sh tests/run_tests.sh)

MYLS="$(pwd)/myls"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Reference ls: C locale (byte order sorting), 512-byte blocks like the manual.
# The manual says -A is always set for the super-user, so mimic that for root.
if [ "$(id -u)" = 0 ]; then ROOT_A=-A; else ROOT_A=; fi
# GNU ls (Linux) differs from NetBSD ls in spacing, "-h" and the "-R" header.
if ls --version 2>&1 | grep -q GNU; then IS_GNU=1; else IS_GNU=0; fi
sys_ls() { LC_ALL=C POSIXLY_CORRECT=1 ls $ROOT_A "$@"; }
my_ls()  { LC_ALL=C "$MYLS" "$@"; }

# ---- build the test tree with fixed, distinct timestamps ----
mkdir -p "$TMP/t/subdir" "$TMP/t/emptydir"
cd "$TMP/t" || exit 1
echo "hello"                 > subdir/data.txt
echo "a"                     > file1.txt
printf 'bb bb bb bb bb\n'    > file2.txt
dd if=/dev/zero bs=3000 count=1 2>/dev/null > big.bin
: > .hidden
chmod +x file1.txt
ln -s file1.txt link.txt
ln -s missing   broken
ln -s subdir    dirlink
mkfifo pipe
touch -h -t 202401011000 file1.txt
touch -h -t 202402011000 file2.txt
touch -h -t 202403011000 big.bin
touch -h -t 202404011000 .hidden
touch -h -t 202405011000 link.txt
touch -h -t 202406011000 broken
touch -h -t 202407011000 dirlink
touch -h -t 202408011000 pipe
touch -h -t 202409011000 subdir/data.txt
touch -h -t 202410011000 subdir
touch -h -t 202411011000 emptydir
touch -h -t 202412011000 .
cd "$TMP" || exit 1

pass=0; fail=0
check() {
    # check "<options and operands>"
    sys_ls $1 > sys.out 2>/dev/null
    my_ls  $1 > my.out  2>/dev/null
    if [ "$IS_GNU" = 1 ]; then
        case "$1" in *-*R*) sed 1d sys.out > sys.tmp; mv sys.tmp sys.out ;; esac
        sed '/^total 0$/d' sys.out > sys.tmp; mv sys.tmp sys.out   # GNU prints it for empty dirs
        sed 's/  */ /g' sys.out > sys.tmp; mv sys.tmp sys.out
        sed 's/  */ /g' my.out  > my.tmp;  mv my.tmp my.out
    fi
    if cmp -s sys.out my.out; then
        pass=$((pass + 1)); printf 'PASS  ls %s\n' "$1"
    else
        fail=$((fail + 1)); printf 'FAIL  ls %s\n' "$1"
        diff sys.out my.out | head -8
    fi
}

for opts in "" -a -A -l -la -lA -lh -F -R -lR -S -lS -t -lt -lr -ltr -lu -lc \
            -i -li -d -ld -n -ln -f; do
    [ "$IS_GNU" = 1 ] && [ "$opts" = -lh ] && continue
    check "$opts t"
done
check "-d t t/subdir"
check "t/file1.txt t/subdir"
check "-l t/file2.txt t/file1.txt"
check "t/subdir t/emptydir"
check "-R t/emptydir"
check "-ld t/dirlink"
check "t/dirlink"

# ---- robustness: must not crash, must return non-zero on error ----
for args in "nonexistent" "t/broken" "-l t/broken" "-R t" "-lR t/emptydir" "-z"; do
    my_ls $args > /dev/null 2>&1
    rc=$?
    if [ "$rc" -ge 128 ]; then
        fail=$((fail + 1)); printf 'FAIL  crash (signal %s) on: myls %s\n' "$rc" "$args"
    else
        pass=$((pass + 1)); printf 'PASS  no crash: myls %s (exit %s)\n' "$args" "$rc"
    fi
done

# ---- exit status ----
my_ls nonexistent > /dev/null 2>&1 && { fail=$((fail+1)); echo "FAIL  exit status for missing file"; } \
                                    || { pass=$((pass+1)); echo "PASS  exit status != 0 for missing file"; }

# ---- non-printable characters: -q shows '?', -w keeps the byte ----
mkdir nonprint; touch "$(printf 'nonprint/a\001b')"
q=$(my_ls -q nonprint); w=$(my_ls -w nonprint | od -c | head -1)
[ "$q" = "a?b" ] && { pass=$((pass+1)); echo "PASS  -q prints ?"; } \
                 || { fail=$((fail+1)); echo "FAIL  -q prints ? (got '$q')"; }
echo "$w" | grep -q '001' && { pass=$((pass+1)); echo "PASS  -w keeps raw byte"; } \
                          || { fail=$((fail+1)); echo "FAIL  -w keeps raw byte"; }

echo "----"; echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
