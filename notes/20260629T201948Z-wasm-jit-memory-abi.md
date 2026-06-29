# 20260629T201948Z - WASM JIT memory import ABI

Base commit: 68b7fa99

Added explicit JIT compile metadata for lowered trace modules that import
`env.memory`. The guest and Wasmtime mirror now share:

- `LJ_WASM_JIT_F_IMPORT_ENV_MEMORY`
- `LJ_WASM_JIT_MEMORY_F_64`
- `LJ_WASM_JIT_MEMORY_F_HAS_MAX`
- `LJWasmJITModule.memory_min`
- `LJWasmJITModule.memory_max`
- `LJWasmJITModule.memory_flags`

`lj_wasm_jit_compile_trace()` sets `IR_LOWERED | IMPORT_ENV_MEMORY` and reports
`memory_min = 1`, `memory_max = 0`, and `memory_flags = MEMORY_F_64` for the
current memory-backed SLOAD modules. NYI modules leave the memory descriptor
zeroed.

The Wasmtime C scaffold contract test now verifies these fields are preserved
through `lj_wasm_import_jit_compile()`. The README and Rust pseudocode document
the host requirement: a real Wasmtime integration must instantiate trace modules
with the running LuaJIT instance's existing memory64 object in the same store,
not a fresh memory.

Validation:

- `make -C src -j$(nproc)`
- `make -C wasm/host/wasmtime check`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
