# 2026-06-29T19:31:22Z wasm64 core compile checkpoint

- Commit at start: `ece260297a95e13eb5b60f1a6707fb173cbd9d20`
- Branch: `wasm`

## What this slice adds

- Broadened `wasm/tools/Makefile` so `wasm64-support-check` compiles every
  non-backend LuaJIT C file with `emcc -sMEMORY64=1`.
- Excluded only files that are known to require the missing executable backend:
  - `luajit.c`
  - `ljamalg.c`
  - `lj_asm.c`
  - `lj_mcode.c`
  - `lj_gdbjit.c`
- Added WASM handling in `lib_jit.c` for CPU detection and trace-exit-stub
  address queries while the WASM VM/JIT backend is incomplete.
- Added secure WASM PRNG seeding through `getentropy()` from the Emscripten
  sysroot.
- Fixed the WASM `ExitState.spill` element type to match LuaJIT snapshot
  restore expectations.

## Validation

- `make -C wasm/tools check` passed, including the broad wasm64 C compile.
- `make -C src -j$(nproc)` passed.
- `git diff --check` passed.
- Native FFI and callback smoke test passed.
