# 20260629T200237Z - Wasmtime FFI host contract coverage

Base commit: `fd3f3c51`

## Slice

- Extended `wasm/host/wasmtime/tests/host_contract_test.c` to cover FFI host
  import validation and hook delegation.
- Covered `ffi_load`, `ffi_unload`, `ffi_symbol`, `ffi_call`,
  `ffi_callback_new`, `ffi_callback_slot`, and `ffi_callback_free`.
- The test now exercises both JIT and FFI hook families through the same
  `lj_wasmtime_host_set_hooks()` contract.

## Validation

- `make -C wasm/host/wasmtime check`
- `make -C src -j$(nproc)`
- `make -C wasm/tools check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Next

- Add a buildable Wasmtime embedding once the guest module and memory ownership
  model are stable enough to instantiate.
- Extend guest-side FFI lowering to describe argument/result layouts beyond the
  current opaque `CCallState` handoff.
