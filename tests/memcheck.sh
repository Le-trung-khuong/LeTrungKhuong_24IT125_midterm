#!/bin/sh
# Run myls under valgrind on a few representative command lines.
# Used instead of AddressSanitizer on systems where ASan refuses to start
# (NetBSD with ASLR enabled). Skips politely if valgrind is not installed.

if ! command -v valgrind >/dev/null 2>&1; then
    echo "valgrind not installed - skipping (pkgin/pkg_add valgrind, or apt install valgrind)"
    exit 0
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/d/sub"; : > "$TMP/d/a"; ln -s a "$TMP/d/l"
fail=0
for args in "" "-lR" "-laiSsh" "-CFG" "-ltu" "-nrf" "-Rd" "nonexistent"; do
    valgrind -q --leak-check=full --error-exitcode=99 ./myls $args "$TMP/d" \
        > /dev/null 2>"$TMP/vg.err"
    if [ $? -eq 99 ]; then
        echo "FAIL  valgrind: myls $args"; cat "$TMP/vg.err"; fail=1
    else
        echo "PASS  valgrind clean: myls $args"
    fi
done
exit $fail
