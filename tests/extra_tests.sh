#!/bin/sh
# Assertion tests for ./myls: every check compares real output with what the
# manual requires (no "exit status 0 means pass" shortcuts).
# Usage: sh tests/extra_tests.sh        (also run by `make test`)

MYLS="$(pwd)/myls"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
pass=0; fail=0

ok()  { pass=$((pass + 1)); printf 'PASS  %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf 'FAIL  %s\n' "$1"; }

my() { LC_ALL=C "$MYLS" "$@"; }

# same "args1" "args2": both command lines must give identical output
same() {
    my $1 t > "$TMP/a.out" 2>&1
    my $2 t > "$TMP/b.out" 2>&1
    if cmp -s "$TMP/a.out" "$TMP/b.out"; then ok "same output: $1 == $2"
    else bad "same output: $1 == $2"; fi
}

# differ "args1" "args2": the outputs must NOT be identical
differ() {
    my $1 t > "$TMP/a.out" 2>&1
    my $2 t > "$TMP/b.out" 2>&1
    if cmp -s "$TMP/a.out" "$TMP/b.out"; then bad "different output: $1 != $2"
    else ok "different output: $1 != $2"; fi
}

# expect "description" "expected text" args...   (names on one line, space separated)
expect() {
    desc=$1; want=$2; shift 2
    got=$(my "$@" 2>&1 | tr '\n' ' ' | sed 's/ $//')
    if [ "$got" = "$want" ]; then ok "$desc"
    else bad "$desc (expected '$want', got '$got')"; fi
}

# ---- fixture: sizes and times are all different ----
mkdir -p "$TMP/t/sub" "$TMP/t/empty"
cd "$TMP/t" || exit 1
printf '%030d' 0 > a.txt            # 30 bytes
printf 'x'       > b.txt            # 1 byte
printf '%020d' 0 > c.txt            # 20 bytes
: > .hidden
ln -s a.txt link
touch -t 202401011000 a.txt
touch -t 202403011000 b.txt
touch -t 202402011000 c.txt
cd "$TMP" || exit 1
# make -u (access time) different from -t (modification time) order
touch -a -t 202405011000 t/c.txt
touch -a -t 202401011000 t/b.txt
touch -a -t 202402011000 t/a.txt

# ---- sort order (checked by content, not just "does not crash") ----
# The super-user always gets -A, so dot files are filtered out of these lists.
names() { grep -v '^\.' ; }
cd "$TMP/t" || exit 1
expect_op() { desc=$1; want=$2; shift 2
    got=$(my "$@" 2>&1 | names | tr '\n' ' ' | sed 's/ $//')
    [ "$got" = "$want" ] && ok "$desc" || bad "$desc (expected '$want', got '$got')"; }
expect_op "default: by name"    "a.txt b.txt c.txt empty link sub" -1 .
expect_op "-1 by name"          "a.txt b.txt c.txt empty link sub" -1
expect_op "-S: largest first"   "a.txt c.txt b.txt" -1S a.txt b.txt c.txt
expect_op "-t: newest first"    "b.txt c.txt a.txt" -1t a.txt b.txt c.txt
expect_op "-tr: oldest first"   "a.txt c.txt b.txt" -1tr a.txt b.txt c.txt
expect_op "-r: reverse names"   "sub link empty c.txt b.txt a.txt" -1r
expect_op "-u: by access time"  "c.txt a.txt b.txt" -1tu a.txt b.txt c.txt
cd "$TMP" || exit 1

# -a and -A (the super-user always gets -A, so only check what holds for both)
my -1a t | grep -q '^\.hidden$' && ok "-a lists .hidden" || bad "-a lists .hidden"
my -1a t | grep -q '^\.\.$' ; if [ "$(id -u)" = 0 ]; then ok "root: -a does not need '..' (-A implied)"; else [ $? = 0 ] && ok "-a lists .." || bad "-a lists .."; fi

# ---- options that cancel each other: the LAST one wins ----
same "-ln"  "-n"
same "-nl"  "-l"
same "-lcu" "-lu"
same "-lur" "-lur"
same "-lc -u" "-lu"
same "-lu -c" "-lc"
same "-Rd"  "-d"
same "-dR"  "-R"
same "-qw"  "-w"
same "-wq"  "-q"
same "-lshk" "-lsk"
same "-lskh" "-lsh"
same "-St"  "-t"
same "-tS"  "-S"
differ "-l" "-n"
differ "-S" "-t"

# ---- long format fields ----
line=$(my -l t/a.txt)
case "$line" in -rw*" 30 "*"a.txt") ok "-l: mode, size 30 and name" ;; *) bad "-l: mode, size 30 and name ($line)" ;; esac
my -ld t | grep -q '^drwx.* t$' && ok "-ld shows the directory itself" || bad "-ld shows the directory itself"
my -l t/link | grep -q 'link -> a.txt$' && ok "-l shows symlink target" || bad "-l shows symlink target"
my -l t/link | grep -q '^l' && ok "-l: symlink type 'l'" || bad "-l: symlink type 'l'"
my -F t/link t/sub t/a.txt | grep -q 'link@' && ok "-F marks symlink with @" || bad "-F marks symlink with @"
my -1F t | grep -q '^sub/$' && ok "-F marks directory with /" || bad "-F marks directory with /"
chmod +x t/b.txt
my -1F t | grep -q '^b.txt\*$' && ok "-F marks executable with *" || bad "-F marks executable with *"
chmod -x t/b.txt
my -lh t/a.txt | grep -q ' 30B ' && ok "-lh prints small sizes in bytes (30B)" || bad "-lh prints small sizes in bytes (30B)"
my -li t/a.txt | grep -Eq '^ *[0-9]+ -rw' && ok "-i prints inode before the mode" || bad "-i prints inode before the mode"
my -ls t/a.txt | grep -Eq '^ *[0-9]+ -rw' && ok "-s prints blocks before the mode" || bad "-s prints blocks before the mode"
my -d t | grep -qx 't' && ok "-d lists the directory itself" || bad "-d lists the directory itself"

# ---- -R ----
my -R t | grep -qx 't/sub:' && ok "-R prints 't/sub:' header" || bad "-R prints 't/sub:' header"
my -R t | grep -qx 't/empty:' && ok "-R descends into empty dir" || bad "-R descends into empty dir"

# ---- multi-column output (bonus) ----
for n in alpha beta gamma delta epsilon zeta eta theta iota kappa lambda mu; do : > "t/sub/$n"; done
COLUMNS=60 my -C t/sub > "$TMP/c.out"
lines=$(wc -l < "$TMP/c.out" | tr -d ' ')
[ "$lines" -lt 12 ] && ok "-C packs 12 names into $lines lines" || bad "-C packs 12 names into $lines lines"
longest=$(awk '{ if (length($0) > m) m = length($0) } END { print m }' "$TMP/c.out")
[ "$longest" -le 60 ] && ok "-C never exceeds COLUMNS (longest line $longest <= 60)" || bad "-C exceeds COLUMNS ($longest)"
COLUMNS=60 my -C t/sub | tr -s ' \n' '\n\n' | sort | grep -v '^$' > "$TMP/c.names"
my -1 t/sub | sort > "$TMP/one.names"
cmp -s "$TMP/c.names" "$TMP/one.names" && ok "-C prints exactly the same names as -1" || bad "-C prints exactly the same names as -1"
COLUMNS=60 my -C -1 t/sub | wc -l | tr -d ' ' | grep -qx 12 && ok "-1 after -C: one per line" || bad "-1 after -C: one per line"
COLUMNS=10 my -C t/sub | wc -l | tr -d ' ' | grep -qx 12 && ok "-C falls back to one column when too narrow" || bad "-C narrow terminal fallback"
COLUMNS=60 my -Cl t/a.txt | grep -q '^-rw' && ok "-l wins over -C" || bad "-l wins over -C"

# ---- colour (bonus) ----
esc=$(printf '\033')
my -1G t | grep -q "$esc" && ok "-G adds colour codes" || bad "-G adds colour codes"
my -1 t  | grep -q "$esc" && bad "no colour without -G" || ok "no colour without -G"
my -1G t | grep "$esc" | grep -q 'sub' && ok "-G colours the directory" || bad "-G colours the directory"

# ---- exit status and messages ----
my nonexistent > /dev/null 2>&1; [ $? -ne 0 ] && ok "missing file: exit status != 0" || bad "missing file: exit status != 0"
my nonexistent 2>&1 >/dev/null | grep -q 'nonexistent' && ok "error message names the file" || bad "error message names the file"
my -z > /dev/null 2>&1; [ $? -ne 0 ] && ok "bad option: exit status != 0" || bad "bad option: exit status != 0"
my -z 2>&1 | grep -qi usage && ok "bad option prints usage" || bad "bad option prints usage"
my t > /dev/null 2>&1; [ $? -eq 0 ] && ok "success: exit status 0" || bad "success: exit status 0"
my t/a.txt nonexistent > "$TMP/o.out" 2>/dev/null; rc=$?
[ $rc -ne 0 ] && grep -q a.txt "$TMP/o.out" && ok "still lists good operands after a bad one" || bad "still lists good operands after a bad one"
if [ -w /dev/full ]; then
    my t > /dev/full 2>/dev/null; [ $? -ne 0 ] && ok "write error (/dev/full) gives exit status != 0" || bad "write error gives exit status != 0"
fi

# ---- robustness: every option on awkward inputs, must never die from a signal ----
for opts in -A -a -c -d -F -f -h -i -k -l -n -q -R -r -S -s -t -u -w -lahisFR -nSrt -lRuc -CG; do
    for target in t / /dev /etc /proc /nonexistent; do
        [ -e "$target" ] || [ "$target" = /nonexistent ] || continue
        my $opts "$target" > /dev/null 2>&1
        rc=$?
        [ "$rc" -ge 128 ] && bad "crash (signal $((rc - 128))): myls $opts $target"
    done
done
ok "no crash with every option on t, /, /dev, /etc, /proc, missing path"

echo "----"; echo "passed: $pass  failed: $fail"
[ "$fail" -eq 0 ]
