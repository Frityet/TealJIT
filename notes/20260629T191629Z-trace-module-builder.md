# 2026-06-29T19:16:29Z trace module builder checkpoint

- Commit at start: `7f4fa8bd720f1f3f8a78b132706009377dee2387`
- Branch: `wasm`

## What this slice adds

- `src/lj_wasm_jit.h` and `src/lj_wasm_jit.c` build a minimal valid Wasm trace
  module that exports `entry(i64 lua_state, i64 base, i32 exitno) -> i32` and
  returns the placeholder `NYI` status.
- `wasm/tools/wasm_nyi_trace_module.c` emits that placeholder trace module from
  a native LuaJIT build.
- `wasm/tools/Makefile` adds a `check` target that emits the module and asks
  `llvm-objdump` to parse it.

This gives the host-assisted JIT path real Wasm bytes to compile while trace IR
lowering is developed.

## Validation

- `make -C src -j$(nproc)` passed.
- `make -C wasm/tools check` passed and produced a `.wasm` file accepted by
  `llvm-objdump -h`.
