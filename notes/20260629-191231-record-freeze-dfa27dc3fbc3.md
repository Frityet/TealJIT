Timestamp: 2026-06-29T19:12:31Z
Commit: dfa27dc3fbc3
Agent: Codex main

Subject: Strict returned-record runtime freeze

Implemented the first strict-mode runtime freeze for Teal records returned from
`.tl` chunks. This targets the explicit module export pattern:

```
local record export end
function export.add(a: integer, b: integer): integer
  return a + b
end
return export
```

The parser now emits a call to `__tealjit_freeze_record(record)` before a
strict single-value return whose static Teal type is `record`. The helper is
registered during base library initialization so dumped Teal bytecode can still
run in a fresh process without reparsing `.tl` source.

The helper installs a protected metatable with `__newindex` that raises
`Teal: frozen record cannot add field`. This preserves LuaJIT 2.1 bytecode
compatibility and uses normal table/metatable semantics: adding a missing field
is rejected, while replacing an existing non-nil field still follows ordinary
Lua table assignment. Existing user metatables are preserved for now; merging
freeze behavior with user metatables needs a later shape-aware helper.

Added test coverage in `test/tealjit/run.sh`:
- strict returned record rejects `m.extra = 1`;
- strict returned record still permits `m.add = m.add`;
- `-t strict=off` leaves the returned record extensible.

Verified:
- `make -C src clean && make -C src -j2`
- `cd test/tealjit && ./run.sh`
- from `src/`: `./luajit -b ../test/tealjit/export_record.tl /tmp/tealjit-export-record.ljbc`
- from `src/`: loaded `/tmp/tealjit-export-record.ljbc` in a fresh process and confirmed adding a new field fails with the frozen-record error
- `make -C src clean && make -C src amalg -j2`
- `cd test/tealjit && ./run.sh`
