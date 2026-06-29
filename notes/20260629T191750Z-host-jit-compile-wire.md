# 2026-06-29T19:17:50Z host JIT compile wiring checkpoint

- Commit at start: `e5c946b845f53d8d44319bd6bdaec306923ead6e`
- Branch: `wasm`

## What this slice adds

- `lj_wasm_jit_compile_nyi()` is available under `LJ_TARGET_WASM`.
- It builds the placeholder trace module, fills `LJWasmJITModule`, and calls
  `lj_wasm_host_jit_compile()`.
- Native builds continue to compile only the target-independent module builder.

This connects generated trace-module bytes to the host-assisted JIT contract.

## Validation

- `make -C src -j$(nproc)` passed.
- `make -C wasm/tools check` passed.
