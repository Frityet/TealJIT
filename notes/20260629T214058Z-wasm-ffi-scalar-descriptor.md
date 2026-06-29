# 20260629T214058Z WASM FFI Scalar Descriptor

Base commit: `6cd5112fac7ead5d7966a9706df9222aaf0f3979`

## What Changed

- Added `LJWasmFFICall`, `LJWasmFFISlot`, and `LJWasmFFILoc` to the WASM host
  ABI. Each slot describes scalar type, call-frame location, byte offset within
  `CCallState`, original CType size, and unsignedness.
- Changed the guest `lj_wasm_host_ffi_call()` import path to pass this descriptor
  alongside `CTState`, `CType`, and `CCallState`.
- Taught the WASM branch of `ccall_set_args()` to populate descriptor slots for
  scalar integer, float, pointer, array-decayed, and function-pointer arguments,
  and to mark varargs/aggregate/by-reference cases with
  `LJ_WASM_FFI_SIG_F_UNSUPPORTED`.
- Updated the Wasmtime C scaffold, hook table, contract test, README, and Rust
  pseudocode so an embedding host can read scalar args/results by descriptor
  offset without reverse-engineering LuaJIT's CType graph first.

## Why

The existing WASM FFI import only handed opaque LuaJIT internals to the host.
That made the smallest real FFI implementation depend on decoding `CType` and
`CCallState` layouts from scratch in the host. The descriptor creates a stable
v0 bridge for explicit-library scalar calls: the host can reject unsupported
descriptors, marshal scalar slots, perform the native call, and write the result
back to the described return slot.

## Validation

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C wasm/host/wasmtime check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
- `git diff --check`

## Next Edge

Implement the host side of scalar explicit-library calls using the descriptor.
The intended first smoke is `ffi.load(...)` plus fixed-arity integer/double
functions; `ffi.C`, varargs, aggregates, and callbacks should remain rejected
until the host has richer symbol and CType handling.
