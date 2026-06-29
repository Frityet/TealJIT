# 20260629T200024Z - Wasmtime host contract test

Base commit: `65329dd6`

## Slice

- Added `wasm/host/wasmtime/tests/host_contract_test.c`.
- The test exercises JIT host import validation and hook delegation for:
  `jit_compile`, `jit_enter`, `jit_patch_exit`, `jit_free`, hook clearing, and
  status-name mapping.
- Updated the Wasmtime host Makefile so `make -C wasm/host/wasmtime check`
  builds and runs the contract test in addition to the syntax check.
- Updated the Wasmtime host README to describe the test.

## Validation

- `make -C wasm/host/wasmtime check`
- `make -C src -j$(nproc)`
- `make -C wasm/tools check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Next

- Add similar host contract coverage for FFI library/symbol/callback imports.
- Replace the Rust pseudocode with a buildable Wasmtime host once the guest VM
  module shape is stable enough to instantiate.
