# 20260629T212509Z - WASM GE guard and descending loop fixture

Base commit: d29e50414f847d08f92a9d500d9c2b732d85e6ad

This slice adds `IR_GE` support to the WASM trace lowerer. `IR_LE` and `IR_GE`
now share the ordered guard path and choose `i32.le_s`/`f64.le` or
`i32.ge_s`/`f64.ge` based on the IR opcode.

Added `wasm/tools/wasm_trace_numeric_for_down_module.c`, a descending numeric
for-loop fixture that matches real LuaJIT IR for:

```lua
for i = 100, 1, -1 do ... end
```

The tools Makefile now validates the generated module, checks for `i32.ge_s`,
and runs the Wasmtime memory64 smoke in `down` mode. The runtime check verifies
that the loop exits with accumulator `5050.0` and stored next index `0` in the
synthetic `ExitState`.

Validation:

- `make -C src -j$(nproc)`
- no-JIT compile of `src/lj_wasm_jit.c`
- `make -C wasm/tools check`
- `make -C wasm/host/wasmtime check`
- native FFI callback smoke
- `git diff --check`
