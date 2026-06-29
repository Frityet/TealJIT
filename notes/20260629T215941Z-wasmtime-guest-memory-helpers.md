# 20260629T215941Z Wasmtime Guest Memory Helpers

Base commit: `29baa186a0c4663414b5b5625065d21aa4eadafb`

## What Changed

- Added `LJWasmtimeGuestMemory`, a simple host-side memory view for WASM64
  imports.
- Added checked helpers:
  - `lj_wasmtime_guest_ptr`
  - `lj_wasmtime_guest_read`
  - `lj_wasmtime_guest_write`
  - `lj_wasmtime_guest_read_cstr`
- The helpers perform overflow-safe bounds checks for memory64 offsets and avoid
  direct casts from guest pointers to native pointers.
- Extended the Wasmtime host contract test with read/write, one-past zero-size,
  overflow, bounded string, too-small destination, and missing-NUL cases.
- Updated the Wasmtime README and Rust pseudocode to route future import
  wrappers through these helpers after borrowing memory from `Caller`.

## Why

The C scaffold's raw `lj_wasm_import_*` functions are useful as an ABI and unit
test surface, but a real Wasmtime linker receives `i64` offsets into guest
linear memory. FFI load/symbol/call and JIT compile imports all need the same
checked translation layer before they can safely read names, structs, call
descriptors, module bytes, or write handles/results back to the guest.

This checkpoint establishes that boundary without requiring a Rust or Wasmtime C
SDK in the container.

## Validation

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime check`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime LIBFFI_LIBS= check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`

## Next Edge

Build guest-facing import wrappers around these helpers: read FFI library/symbol
names from guest memory, store opaque handle IDs back into guest memory, and
decode `LJWasmFFICall` from a guest offset before invoking the numeric scalar
libffi backend.
