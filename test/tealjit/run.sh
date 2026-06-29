#!/bin/sh
set -eu

LUAJIT=${LUAJIT:-../../src/luajit}

"$LUAJIT" -e 'local m = dofile("export_record.tl"); assert(m.add(20, 22) == 42)'

if "$LUAJIT" strict_nil_bad.tl >/tmp/tealjit-strict-nil.out 2>&1; then
  echo "strict_nil_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "strict nil safety" /tmp/tealjit-strict-nil.out

"$LUAJIT" strict_off_nil.tl
"$LUAJIT" -t strict=off strict_cli_off.tl

if "$LUAJIT" as_bad.tl >/tmp/tealjit-as-bad.out 2>&1; then
  echo "as_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "invalid strict cast" /tmp/tealjit-as-bad.out
