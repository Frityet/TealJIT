Timestamp: 2026-06-29T19:44:28Z
Commit: c9313eaa4114
Agent: Codex main
Subagent: Rawls (019f14e1-9e4e-7241-8bf6-9a913a487a52)

Subject: Parser-owned function signatures from Teal type annotations

Implemented native parsing for simple Teal function type annotations and wired
them into the parser-only signature metadata added in the previous slice.

What changed:
- `TealTypeDesc` now carries an optional parser-owned function signature id and
  owner `FuncState`.
- The type parser now recognizes `function(...)` annotations and allocates
  `TealFuncSig` entries for them.
- Function type annotations support positional parameters, named parameters,
  named optional parameters like `suffix?: string`, unnamed optional parameters
  like `? string`, and a single annotated return type.
- Local variables, function parameters, and record fields retain signatures from
  function type annotations.
- Declaration-time local assignment and record-field assignment now compare
  known function signatures when both sides are statically known.
- Calls through annotated local function values and annotated record fields use
  the declared signature for arity, argument, and return propagation checks.

Covered in `test/tealjit`:
- `function_type_annotations.tl` exercises local function type annotations,
  named and unnamed optional parameter forms, and record function fields.
- `function_type_annotation_arg_bad.tl` rejects bad calls through annotated
  local function values.
- `function_type_annotation_arity_bad.tl` rejects missing required arguments.
- `function_type_annotation_assign_bad.tl` rejects incompatible function values
  assigned to annotated locals.
- `record_function_type_assignment_bad.tl` rejects incompatible function values
  assigned to declared record function fields.

Rawls' read-only guidance matched this slice: function type annotations should
share the existing parser-owned signature allocator, then thread the signature
through locals, parameters, and record fields. Rawls also called out method
colon-call `self`, varargs, multi-return lists, overloads, generics, and global
record shape tracking as separate follow-ups.

Known follow-ups:
- Generic function type parameters are skipped coarsely rather than modeled.
- Vararg, tuple/multi-return, overload, and generic function semantics are not
  implemented.
- Function subtyping is intentionally conservative and only compares known
  primitive parameter/return types plus arity ranges.
- Colon-call method `self` shape/signature handling remains a separate parser
  slice.

Verified:
- `make -C src clean`
- `make -C src -j2`
- `cd test/tealjit && ./run.sh`
- `make -C src clean`
- `make -C src amalg -j2`
- `cd test/tealjit && ./run.sh`
