# 20260629T220452Z Wasmtime Guest FFI Wrappers

Base commit: `e5fe3f1e9aaf95038e7c594c170e26dc4d9d7d98`

## What Changed

- Added `LJWasmtimeHandleTable` and `LJWasmtimeGuestContext` to the Wasmtime
  host scaffold.
- Added guest-facing FFI helper wrappers:
  - `lj_wasmtime_guest_ffi_load`
  - `lj_wasmtime_guest_ffi_symbol`
  - `lj_wasmtime_guest_ffi_unload`
  - `lj_wasmtime_guest_ffi_call`
- Guest-visible handles are `u64` table IDs. Native library/symbol handles stay
  in the host-side table.
- `lj_wasmtime_guest_ffi_call()` now reads `LJWasmFFICall` and the guest
  `CCallState` from checked guest memory, maps the guest symbol ID at
  `func_offset` to a native host handle only in a temporary call-frame copy,
  invokes the raw `lj_wasm_import_ffi_call()` ABI, restores the guest symbol ID,
  and writes the updated frame back to guest memory.
- Extended the libffi host contract test with an end-to-end translated guest
  FFI call: guest memory contains `libm`, `cos`, a guest symbol ID in the call
  frame, and the descriptor; the wrapper calls `cos(0.0)`, writes back `1.0`,
  preserves the guest handle ID, and rejects the call after unload invalidates
  the owning library handle.
- Updated README and Rust pseudocode to show real Wasmtime import wrappers
  delegating to these guest-facing helpers after borrowing memory from `Caller`.

## Why

The previous checkpoint provided safe memory access and a native scalar backend,
but real Wasmtime imports still receive guest `i64` offsets and guest-visible
handle IDs. These wrappers are the missing adapter layer between a Wasmtime
linker closure and the raw C ABI/test scaffold.

This is still not a complete production host: the actual Wasmtime `Caller`
integration and table lifetime policy need more work. But the core FFI
load/symbol/call translation path is now concrete and covered by C tests.

## Validation

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime check`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime LIBFFI_LIBS= check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`

## Next Edge

Add guest-facing wrappers for JIT compile/enter/patch imports. `jit_compile`
especially needs checked decoding of `LJWasmJITModule`, copying module bytes out
of guest memory, and mapping compiled trace handles to guest-visible IDs.
