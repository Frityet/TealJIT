# 20260629T221438Z Wasmtime Memory64 Runtime FFI Fixture

Base commit: `da3ff6ac038c0d16efe8a7629068b13b16332d34`

## What Changed

- Added a shared-library build target for the Wasmtime C scaffold:
  `liblj_wasmtime_host.so`.
- Added `make -C wasm/host/wasmtime guest-runtime-check`.
- Added `tests/guest_runtime_check.py`, which uses the Python `wasmtime`
  package and `ctypes` to instantiate a real memory64 guest module importing:
  - `lj_wasm_import_ffi_load`
  - `lj_wasm_import_ffi_symbol`
  - `lj_wasm_import_ffi_call`
  - `lj_wasm_import_ffi_unload`
- The fixture links those imports to the C guest wrapper helpers, borrows the
  guest memory through `Caller.get("memory").data_ptr`, and verifies that guest
  offsets and guest-visible handle IDs drive a real scalar FFI call.
- The guest module loads `libm.so.6`, resolves `cos`, writes the guest symbol ID
  into a synthetic `CCallState`, calls `cos(0.0)`, verifies the result `1.0`,
  preserves the guest handle ID in memory, unloads the library, and verifies a
  later call fails.
- `guest-runtime-check` skips cleanly when libffi is not available.

## Why

The previous guest wrappers were covered by C unit tests using an in-process
byte array. This checkpoint proves the wrapper path across an actual Wasmtime
memory64 instance and real imported functions, which is much closer to the host
contract a WASM64 LuaJIT guest will need.

## Validation

- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime check && make -C wasm/host/wasmtime guest-runtime-check`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime LIBFFI_LIBS= check && make -C wasm/host/wasmtime LIBFFI_LIBS= guest-runtime-check`
- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`

## Next Edge

Add a similar memory64 Wasmtime runtime fixture for the JIT wrappers, then start
turning the pseudocode into a concrete host harness that can compile and enter
trace modules from a guest instance.
