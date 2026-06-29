# 20260629T212006Z - WASM guard exit-state stores

Base commit: d7c15317be5ce187db79634147f5c9b17851a69a

This slice makes guard exits write trace values into the `ExitState` pointer
passed through the new JIT-entry ABI before returning a non-negative exit ID.

Implementation notes:

- Added Wasm store opcodes and signed `i64.extend_i32_s` support to the binary
  emitter.
- Added `lj_wasm_jit_assign_exitstate(GCtrace *T)`, which gives value-producing
  IR refs deterministic WASM64 exit locations: 16 GPR/reference slots, 16 FPR
  slots, then spill slots.
- The WASM assembler calls that assignment before compiling a trace module, so
  `lj_snap_restore()` can later read from the same `ir->prev` map.
- Guard exits write snapshot-live refs when snapshots are available. The
  standalone fixtures have no snapshots, so they write all assigned value refs.

Validation evidence:

- `make -C wasm/tools check` passed.
- The numeric-loop Wasmtime smoke now reads `ExitState` memory after exit `2`
  and checks accumulator `5050.0` plus loop index `101`.
- `wasm-objdump` shows `i64.store` and `f64.store` blocks immediately before
  guard returns; the loop-body exit writes offset `32` for the accumulator and
  offset `144` for the loop index before returning `i32.const 2`.

Remaining gap: `lj_vm_wasm_trace_enter()` still returns the trace entry status
directly. It can be wired to call `lj_vm_wasm_trace_exit()` for non-negative
statuses once the host entry implementation is executing these modules in the
same memory and passing the populated `ExitState` back.
