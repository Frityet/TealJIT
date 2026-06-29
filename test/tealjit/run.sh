#!/bin/sh
set -eu

LUAJIT=${LUAJIT:-../../src/luajit}

"$LUAJIT" -e 'local m = dofile("export_record.tl"); assert(m.add(20, 22) == 42)'
"$LUAJIT" -e 'local m = dofile("export_record.tl"); m.add = m.add; local ok, err = pcall(function() m.extra = 1 end); assert(not ok and tostring(err):match("frozen record"))'
"$LUAJIT" -t strict=off -e 'local m = dofile("export_record.tl"); m.extra = 1; assert(m.extra == 1)'

if "$LUAJIT" strict_nil_bad.tl >/tmp/tealjit-strict-nil.out 2>&1; then
  echo "strict_nil_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "strict nil safety" /tmp/tealjit-strict-nil.out

"$LUAJIT" strict_off_nil.tl
"$LUAJIT" -t strict=off strict_cli_off.tl
"$LUAJIT" is_primitive.tl
"$LUAJIT" record_fields.tl

if "$LUAJIT" as_bad.tl >/tmp/tealjit-as-bad.out 2>&1; then
  echo "as_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "invalid strict cast" /tmp/tealjit-as-bad.out

if "$LUAJIT" is_name_only_bad.tl >/tmp/tealjit-is-name-only.out 2>&1; then
  echo "is_name_only_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "can only use 'is' on variables" /tmp/tealjit-is-name-only.out

if "$LUAJIT" record_field_unknown_read_bad.tl >/tmp/tealjit-record-read.out 2>&1; then
  echo "record_field_unknown_read_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-record-read.out

if "$LUAJIT" record_field_unknown_write_bad.tl >/tmp/tealjit-record-write.out 2>&1; then
  echo "record_field_unknown_write_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "unknown record field" /tmp/tealjit-record-write.out

if "$LUAJIT" record_field_type_bad.tl >/tmp/tealjit-record-type.out 2>&1; then
  echo "record_field_type_bad.tl unexpectedly passed" >&2
  exit 1
fi
grep -q "record field type mismatch" /tmp/tealjit-record-type.out
