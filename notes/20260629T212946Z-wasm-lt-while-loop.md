# 20260629T212946Z - WASM LT/GT guards and numeric while fixture

Base commit: f9bc6e88a3f58ba91210fc09e8741c31d7eba726

This slice extends the ordered guard lowerer from `LE`/`GE` to all four ordered
comparisons: `LT`, `LE`, `GT`, and `GE`. The lowerer now chooses the matching
signed integer or f64 Wasm comparison opcode for each IR guard.

Added `wasm/tools/wasm_trace_numeric_while_module.c`, a numeric while-loop
fixture matching real LuaJIT IR that uses f64 `LT` guards and num PHIs. The
runtime smoke gained a `while` mode that verifies the loop exits with:

- exit ID `3` for the loop-body `LT` guard,
- accumulator `5050.0` stored in FPR slot 5,
- loop index `100.0` stored in FPR slot 4.

Validation:

- `make -C src -j$(nproc)`
- no-JIT compile of `src/lj_wasm_jit.c`
- `make -C wasm/tools check`
- `make -C wasm/host/wasmtime check`
- native FFI callback smoke
- `git diff --check`
