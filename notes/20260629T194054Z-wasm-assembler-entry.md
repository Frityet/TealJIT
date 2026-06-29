# 20260629T194054Z - WASM assembler entry

Base commit: `dcfc2b84`

## Slice

- Added a `LJ_TARGET_WASM` branch in `lj_asm.c` instead of forcing the native reverse-emission assembler to compile for WebAssembly.
- The WASM assembler entry currently routes trace assembly through `lj_wasm_jit_compile_nyi()`, reserves a one-byte mcode token for LuaJIT bookkeeping, and stores the host compiled-trace handle separately on `GCtrace`.
- Added host patch-exit/free lifecycle plumbing through `lj_wasm_host_jit_patch_exit()` and `lj_wasm_host_jit_free()`.
- Mirrored the host handle into `J->curfinal` so trace-abort cleanup can release it after post-assembly failures.
- Added `LJ_TRERR_WASMJIT` for host-side trace compiler failures.
- Wired WASM support modules into `ljamalg.c` for WASM builds.
- Removed all skipped source files from `wasm/tools` wasm64 support compilation. The amalgam is still compiled with a targeted `-Wno-unused-function` because the single translation unit makes internal helper symbols look unused while the WASM assembler path bypasses native helpers.

## Validation

- `emcc -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I src -c src/lj_asm.c -o /tmp/lj_asm.o`
- `emcc -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I src -c src/lj_trace.c -o /tmp/lj_trace.o`
- `make -C src clean && make -C src amalg -j$(nproc) && make -C src clean && make -C src -j$(nproc)`
- `make -C wasm/tools check`
- `make -C wasm/host/wasmtime check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Next

- Replace the NYI trace module with a minimal real IR-to-Wasm lowering path.
- Add host-entry integration for entering compiled trace handles from the WASM VM surface.
