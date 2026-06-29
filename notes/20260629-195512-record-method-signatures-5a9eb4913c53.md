Timestamp: 2026-06-29T19:55:12Z
Commit: 5a9eb4913c53
Agent: Codex main

Subject: Record method signatures and self shape checks

Implemented the first native method-signature checking pass for local Teal
records.

What changed:
- `bcemit_method()` now looks up the receiver record shape before bytecode
  emission and attaches the record field's static function signature to the
  method expression.
- `parse_args()` now accepts an implicit argument count. Colon calls pass one
  implicit receiver, so explicit arguments are checked against parameters after
  `self`, and arity checks include the receiver.
- Dot calls to method fields keep checking the explicit first argument, so
  missing `self` is rejected as a function argument mismatch.
- Dot-field parsing now captures record shape metadata before register
  discharge and clears stale receiver type metadata after creating the indexed
  expression.
- Colon method definitions clone the receiver's parser-only record shape into
  the child function state, so `self.field` inside `function R:method(...)` is
  type-checked against the same local record fields.
- The Teal type parser now stops a function type before a following `name:`
  record field, so newline-separated function fields no longer require
  semicolons.

Covered in `test/tealjit`:
- `record_method_colon_call.tl` checks colon calls, equivalent dot calls with
  explicit receiver, and `self.value` inside a method body.
- `record_method_colon_arg_bad.tl` rejects a bad explicit method argument.
- `record_method_dot_missing_self_bad.tl` rejects a dot call missing `self`.
- `record_method_unknown_bad.tl` rejects unknown method lookup in strict mode.
- `record_method_self_unknown_bad.tl` rejects unknown `self` field access in a
  method body.

Known follow-ups:
- Receiver shape ownership is still parser-local and cloned only for local
  record method definitions in the current function state.
- Global record shapes, cross-function record shape ownership, interface/record
  type aliases, overloads, generics, and vararg method signatures remain
  follow-up work.

Verified:
- `make -C src clean`
- `make -C src -j2`
- `cd test/tealjit && ./run.sh`
- `make -C src clean`
- `make -C src amalg -j2`
- `cd test/tealjit && ./run.sh`
