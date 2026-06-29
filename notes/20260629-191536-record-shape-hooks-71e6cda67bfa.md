# Teal record shape hook inspection

Date: 2026-06-29 19:15:36 UTC
Commit inspected: 71e6cda67bfa
Branch: v2.1

Scope: inspection only. No source edits.

## Current relevant state

- `ExpDesc` only carries coarse Teal type state today: `teal_type` and `teal_nil` at `src/lj_parse.c:55-70`.
- `FuncState` only carries per-local coarse type/nilability arrays at `src/lj_parse.c:145-173`.
- Local lookup copies those coarse local tags into expressions at `src/lj_parse.c:1509-1520`.
- `expr_index()` destroys the source key context into bytecode-oriented `aux` encodings at `src/lj_parse.c:2035-2064`.
- `expr_field()` is the last point where a dot field has both the base expression and exact `GCstr *` key before indexing at `src/lj_parse.c:2066-2075`.
- `expr_discharge()` is the central read emission point for `VINDEXED` expressions at `src/lj_parse.c:471-489`.
- `bcemit_store()` is the central store emission point for `VINDEXED` writes at `src/lj_parse.c:650-692`.
- `parse_assignment()` currently type-checks only local LHS values and only has the last RHS expression in rich `ExpDesc` form for exact arity assignment at `src/lj_parse.c:2629-2669`.
- `parse_func()` parses `function export.add(...)` as a dotted LHS and stores the function body through `bcemit_store()` at `src/lj_parse.c:2811-2829`.
- Teal record declarations are currently coarse table creation only: local records at `src/lj_parse.c:2686-2700`, global records at `src/lj_parse.c:2703-2719`.

## Safest hook map

1. Add parser-only record shape metadata beside the existing Teal tags.
   - Put a shape pointer or shape id on `ExpDesc` next to `teal_type/teal_nil`.
   - Put `teal_vshape[LJ_MAX_LOCVAR]` on `FuncState` next to `teal_vtype/teal_vnil`.
   - Initialize these in the same places as the existing Teal metadata, especially `expr_init()` at `src/lj_parse.c:103-111` and `fs_init()` where `teal_vtype/teal_vnil` are cleared at `src/lj_parse.c:2012-2015`.
   - This keeps the metadata parser-only and discarded before `GCproto` finalization.

2. Create shapes in `parse_teal_local_record()` and `parse_teal_global_record()`.
   - `parse_teal_local_record()` is the safest local record ownership point because it has the declared name and destination local register before emitting `BC_TNEW` at `src/lj_parse.c:2686-2696`.
   - `parse_teal_global_record()` needs an additional parser-level global shape map keyed by record name, because it emits a global store but has no local slot to remember the shape at `src/lj_parse.c:2703-2715`.
   - The current `teal_skip_type_decl()` path means any real record body parser must replace the skip for record declarations; empty `record Name end` can still produce an open empty shape.

3. Resolve dot fields in `expr_field()`, but do not eagerly reject unknown fields there.
   - `expr_field()` is the right place to attach field metadata because it has the base expression before indexing and the string key before `expr_index()` compresses it at `src/lj_parse.c:2066-2075`.
   - It should annotate the resulting `VINDEXED` expression with base shape, field name, known/unknown status, field type, and child shape if any.
   - It should not immediately error on an unknown field. The same syntax is used for reads, assignment LHS, and `function export.add(...)`; eager errors here would break the requested module construction pattern.

4. Enforce strict dot-field reads in `expr_discharge()`.
   - `expr_discharge()` is where `VINDEXED` turns into `TGET*` bytecode at `src/lj_parse.c:471-489`.
   - If `ls->teal && ls->teal_strict` and the `VINDEXED` expression carries record-field metadata for an unknown field, error here before emitting `TGET*`.
   - This preserves normal Lua and allows unknown final fields on LHS until a store hook decides whether the write is legal.

5. Enforce strict dot-field writes in `bcemit_store()`.
   - `bcemit_store()` already centralizes `TSET*` emission for indexed stores at `src/lj_parse.c:672-688`.
   - If the `VINDEXED` expression has record-field metadata, check the base shape and field name before emitting the store.
   - For an open construction shape, allow adding the final field and record its type from the RHS.
   - For a closed/frozen shape, reject unknown fields and type-check known fields.
   - Leave dynamic bracket indexes alone initially unless they are constant string keys and explicitly marked as shape-checkable.

6. Treat `parse_func()` as a special write producer.
   - `function export.add(...)` reaches `bcemit_store()` only after parsing the function body at `src/lj_parse.c:2819-2828`.
   - To register `add` as a function field, either mark the body expression as `TEAL_T_FUNCTION` in `parse_body()` or have `parse_func()` pass a function-field descriptor to the same store helper used by `bcemit_store()`.
   - Intermediate dotted components should still be checked as reads. For example, in `function export.sub.add()`, `export.sub` must already be a known record before `add` can be added.

7. Keep all enforcement Teal-gated.
   - Shape checks should be gated by `ls->teal`, and strict errors by `ls->teal_strict`, so normal `.lua` parsing and bytecode generation do not change.
   - Table constructors call `bcemit_store()` internally at `src/lj_parse.c:2157-2160`, so the store hook must only run when record-field metadata is present.

## Risks and follow-ups

- Multi-assignment loses RHS type detail for earlier LHS stores during recursive unwinding at `src/lj_parse.c:2629-2669`. A central `bcemit_store()` hook can enforce field existence, but full field type checking needs a RHS type vector or a staged assignment check.
- Method calls use `bcemit_method()` rather than `expr_field()` for `obj:method()` at `src/lj_parse.c:696-716` and `src/lj_parse.c:2369-2374`; strict method-field reads need a matching hook there.
- `parse_params()` creates `self` as coarse `TEAL_T_RECORD` at `src/lj_parse.c:2208-2211`, but it does not know the owner shape. Method body checking will need owner-shape propagation from `parse_func()` into `parse_body()/parse_params()`.
- Global record shape lookup needs parser-level state beyond `FuncState`, otherwise `global record Foo end; Foo.x = 1` cannot recover the shape from a later `VGLOBAL`.
- Runtime freezing currently installs a metatable helper on strict record returns, but it preserves existing metatables and only blocks missing-key writes. Parser-only shape checking should not assume runtime freeze catches every mutation path.
