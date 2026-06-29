# 2026-06-29T19:26:27Z wasm64 support compile check

- Commit at start: `ae81cd829125de4ff357ee2bca89b8361d1a43ec`
- Branch: `wasm`

## What this slice adds

- `wasm/tools/Makefile` now has `wasm64-support-check`.
- `make -C wasm/tools check` now does two things:
  - emits and validates the placeholder trace module
  - compiles target-independent WASM support objects with
    `emcc -sMEMORY64=1`

The compiled support objects are:

- `src/lj_wasm_emit.c`
- `src/lj_wasm_host.c`
- `src/lj_wasm_jit.c`
- `src/lj_vm_wasm.c`

This gives the port a real wasm64 compile ratchet while the full VM backend is
still gated.

## Validation

- `make -C wasm/tools check` passed.
- `make -C src -j$(nproc)` passed.
- `git diff --check` passed.
- Native FFI and callback smoke test passed.
