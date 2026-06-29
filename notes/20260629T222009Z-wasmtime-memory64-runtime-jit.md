# 20260629T222009Z Wasmtime Memory64 Runtime JIT Fixture

Base commit: `d5d4b7a27dbcee588972917b231d648783cb0fa3`

## What Changed

- Extended `wasm/host/wasmtime/tests/guest_runtime_check.py` so the Python
  Wasmtime memory64 fixture now exercises both FFI and JIT guest-wrapper paths.
- Added a second tiny memory64 guest module that imports:
  - `lj_wasm_import_jit_compile`
  - `lj_wasm_import_jit_enter`
  - `lj_wasm_import_jit_patch_exit`
  - `lj_wasm_import_jit_free`
- Added `ctypes` mirrors for `LJWasmJITModule` and `LJWasmtimeHostHooks` plus
  Python callback hooks for compile, enter, patch, and free.
- The fixture writes module bytes and two `LJWasmJITModule` descriptors into
  real Wasmtime guest memory, calls the imported JIT wrappers, and verifies that
  guest offsets are translated into host-owned trace handles.
- The runtime check now covers compile of two traces, entry into the first
  trace, patching an exit to the second trace, freeing the first trace, and a
  rejected enter after the guest-visible handle has been released.
- Updated `wasm/host/wasmtime/README.md` to document that
  `guest-runtime-check` verifies both FFI and JIT imports through actual
  memory64 modules.

## Why

The C contract test already covered JIT wrapper translation through an
in-process byte array. This checkpoint proves the same wrapper contract through
real Wasmtime memory64 exports/imports, so a future concrete Wasmtime host can
reuse the guest-memory and handle-table boundary with more confidence.

## Validation

- `make -C wasm/host/wasmtime clean`
- `make -C wasm/host/wasmtime check`
- `make -C wasm/host/wasmtime guest-runtime-check`
- `make -C wasm/host/wasmtime clean`
- `make -C wasm/host/wasmtime LIBFFI_LIBS= check`
- `make -C wasm/host/wasmtime LIBFFI_LIBS= guest-runtime-check`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C src -j$(nproc)`
- `make -C wasm/tools clean`
- `make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
- `git diff --check`

## Next Edge

Start turning the Wasmtime linker pseudocode into a concrete host harness that
can compile and enter trace modules from a guest instance, using these
guest-wrapper tests as the host ABI guardrail.
