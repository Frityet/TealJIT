# 20260629T212141Z - WASM trace exit routing

Base commit: 4ad2528c3b293632e5d7dd052db829aabd19caba

This slice wires the guest-side trace-entry helper to the existing LuaJIT
trace-exit restoration path. `lj_vm_wasm_trace_enter()` now treats a
non-negative result from `lj_wasm_host_jit_enter()` as a trace exit number. If
the exit is in range for the parent trace, it calls:

```c
lj_vm_wasm_trace_exit(L, traceno, (ExitNo)status, &ex)
```

The VM state and `jit_base` stay live until after that restoration call returns,
then the helper restores interpreter state bookkeeping as before. Negative
results remain host/backend statuses and pass through unchanged.

Validation:

- `make -C src -j$(nproc)`
- no-JIT compile of `src/lj_wasm_jit.c`
- `make -C wasm/tools check`
- `make -C wasm/host/wasmtime check`
- native FFI callback smoke
- `git diff --check`

Remaining host-side gap: the Wasmtime host scaffold still delegates `jit_enter`
to a hook/default `NYI`; it does not yet instantiate a trace handle and call the
compiled module's `entry` export itself.
