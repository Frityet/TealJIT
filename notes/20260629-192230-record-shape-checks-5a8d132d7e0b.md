Timestamp: 2026-06-29T19:22:30Z
Commit: 5a8d132d7e0b
Agent: Codex main

Subject: Parser-only record shape checks

Implemented the first native record-shape metadata pass in `src/lj_parse.c`.
This is still parser-only metadata and does not change LuaJIT bytecode or dump
format.

What changed:
- `ExpDesc` now carries a static record shape id and dot-field metadata.
- `FuncState` now carries local record shapes and simple string field entries.
- `local record R ... end` parses simple `field: type` entries instead of
  skipping the body.
- Empty local records stay open, so the module construction pattern
  `local record export end; function export.add...; return export` still works.
- Records with declared fields close after their body, so strict writes to
  unknown fields are rejected.
- Strict dot-field reads are checked when indexed expressions discharge.
- Strict dot-field writes are checked before `TSET*` emission.
- Function bodies now stamp their result expression as `function`, allowing
  `function export.add...` to add a typed function field to an open record
  shape.
- Strict record returns close open parser shapes before emitting the runtime
  freeze helper call.

Covered in `test/tealjit`:
- `record_fields.tl` verifies declared field writes/reads and the open export
  function pattern.
- `record_field_unknown_read_bad.tl` rejects unknown declared-record reads.
- `record_field_unknown_write_bad.tl` rejects unknown declared-record writes.
- `record_field_type_bad.tl` rejects field assignment type mismatches.

Known follow-ups:
- Global record shapes are not recovered from later global lookups yet.
- Bracket string keys, table-literal-to-record checks, method `self` shape,
  multi-assignment type vectors, nominal identity across modules, interfaces,
  generics, and nested records still need fuller checker work.

Verified:
- `make -C src clean && make -C src -j2`
- `cd test/tealjit && ./run.sh`
- `make -C src clean && make -C src amalg -j2`
- `cd test/tealjit && ./run.sh`
