# 20260629T211321Z - WASM JIT exit-state ABI

Base commit: b7bddd082208dc18b37d7e6343b4e0436db17600

This slice extends the trace-entry ABI so a host can provide an `ExitState`
storage pointer to compiled trace modules. The generated Wasm entry signature
is now:

```text
(i64 lua_state, i64 base, i64 exit_state, i32 exitno) -> i32
```

The host import wrapper and Wasmtime scaffold now forward the same pointer via
`lj_wasm_host_jit_enter()` / `lj_wasm_import_jit_enter()`. The in-guest
`lj_vm_wasm_trace_enter()` allocates an `ExitState` object and passes its
address to the host entry import.

Validation evidence:

- `wasm-objdump -x /tmp/lj-wasm-numeric-for-trace.wasm` reports
  `type[0] (i64, i64, i64, i32) -> i32`.
- The Python Wasmtime runtime smoke calls the entry with an `exit_state` guest
  pointer.
- The Wasmtime host contract test asserts that `jit_enter` hooks receive the
  exact exit-state pointer.

This is an ABI-prep step only. Trace modules still need to write live locals
into `ExitState` on each guard exit before `lj_vm_wasm_trace_enter()` can safely
route non-negative entry returns through `lj_vm_wasm_trace_exit()`.
