# 20260629T200453Z - WASM trace fallback validation

Base commit: `2d995c52`

## Slice

- Added `wasm/tools/wasm_trace_fallback_module.c`.
- The tool builds a synthetic trace with unsupported `IR_DIV`, confirms
  `lj_wasm_jit_build_trace()` reports `lowered == 0`, and emits the fallback NYI
  module.
- Updated `wasm/tools/Makefile` so `make -C wasm/tools check` validates both the
  supported lowered trace module and the unsupported fallback module with
  `wasm-validate`.

## Validation

- `make -C wasm/tools check`
- `make -C src -j$(nproc)`
- `make -C wasm/host/wasmtime check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Next

- Add semantic memory/TValue lowering for `SLOAD` so the lowered arithmetic path
  stops materializing placeholder zeros.
