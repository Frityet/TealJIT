# 2026-06-29T19:23:16Z wasm callback contract checkpoint

- Commit at start: `016e4e5c367f84c578fd68ba5ae4cdb8efc41255`
- Branch: `wasm`

## What this slice adds

- Added host imports/wrappers for FFI callback handles:
  - `lj_wasm_import_ffi_callback_new`
  - `lj_wasm_import_ffi_callback_slot`
  - `lj_wasm_import_ffi_callback_free`
- `lj_ccallback.c` now has a WASM callback slot path that asks the host for an
  opaque callable handle instead of allocating machine-code trampolines.
- `lib_ffi.c` releases the host callback handle from `callback:free()` on WASM.
- The Wasmtime host scaffold and docs include the callback imports.

## Validation

- `make -C src -j$(nproc)` passed.
- `make -C wasm/host/wasmtime check` passed.
- `make -C wasm/tools check` passed.
- Native FFI callback smoke test passed:
  `ffi.cast("cb_t", function(x) return x+1 end)(41) == 42`.
