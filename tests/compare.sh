#!/bin/sh
# Compare this implementation with the system ls. Differences are reported, not hidden.
set -u

SYSTEM_LS=${SYSTEM_LS:-/bin/ls}
TARGET=${TARGET:-tests/testdata}
TMP_SYSTEM=${TMPDIR:-/tmp}/ls-system.$$
TMP_MINE=${TMPDIR:-/tmp}/ls-mine.$$
trap 'rm -f "$TMP_SYSTEM" "$TMP_MINE"' 0 HUP INT TERM

if [ ! -x ./ls ]; then
    echo "Build first with: make" >&2
    exit 1
fi
if [ ! -x "$SYSTEM_LS" ]; then
    echo "System ls not found at $SYSTEM_LS; set SYSTEM_LS=/path/to/ls" >&2
    exit 1
fi

passed=0
total=0
compare_case() {
    label=$1
    shift
    total=$((total + 1))
    LC_ALL=C TZ=UTC "$SYSTEM_LS" "$@" "$TARGET" >"$TMP_SYSTEM" 2>/dev/null
    sys_status=$?
    LC_ALL=C TZ=UTC ./ls "$@" "$TARGET" >"$TMP_MINE" 2>/dev/null
    mine_status=$?
    if [ "$sys_status" -eq "$mine_status" ] && diff -u "$TMP_SYSTEM" "$TMP_MINE" >/dev/null 2>&1; then
        echo "PASS: $label"
        passed=$((passed + 1))
    else
        echo "DIFF: $label"
        diff -u "$TMP_SYSTEM" "$TMP_MINE" || true
    fi
}

compare_case default
compare_case all -a
compare_case no-sort -f
compare_case quote -q
compare_case raw -w
compare_case almost-all -A
compare_case classify -F
compare_case inode -i
compare_case long -l
compare_case numeric -n
compare_case blocks -s
compare_case kilobyte-blocks -sk
compare_case human-blocks -sh
compare_case reverse -r
compare_case size-sort -S
compare_case time-sort -t
compare_case access-time -tu
compare_case change-time -tc
compare_case long-reverse-time -ltr
compare_case directories-as-files -d
compare_case long-inode -li
compare_case recursive -R
compare_case long-all -la

printf '\nExact-output matches: %s/%s\n' "$passed" "$total"
