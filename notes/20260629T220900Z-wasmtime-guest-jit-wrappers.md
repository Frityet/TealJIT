# 20260629T220900Z Wasmtime Guest JIT Wrappers

Base commit: `813f2bf3252791a48e2ea367ec378de298cb3c3d`

## What Changed

- Added guest-facing JIT wrapper helpers to the Wasmtime host scaffold:
  - `lj_wasmtime_guest_jit_compile`
  - `lj_wasmtime_guest_jit_free`
  - `lj_wasmtime_guest_jit_enter`
  - `lj_wasmtime_guest_jit_patch_exit`
- `jit_compile` reads `LJWasmJITModule` from checked guest memory, copies module
  bytes out of guest memory into a temporary host buffer, calls the raw
  `lj_wasm_import_jit_compile()` ABI, maps the returned compiled trace handle to
  a guest-visible `u64` ID, and writes that ID back to guest memory.
- `jit_enter`, `jit_patch_exit`, and `jit_free` translate guest handle IDs back
  through `LJWasmtimeHandleTable` before calling the raw JIT ABI.
- Extended the host contract test to compile a guest-described trace module,
  enter it, patch it to another guest handle ID, free it, and reject an enter
  through the freed ID.
- Updated the Wasmtime README and Rust pseudocode so future linker closures can
  delegate JIT imports to the guest wrapper helpers.

## Why

The previous guest FFI wrappers established checked memory and handle-ID
translation for dynamic calls. JIT imports need the same boundary: a real
Wasmtime closure receives guest offsets and integer handles, while the raw C
contract wants native pointers/handles and a stable module byte buffer.

This checkpoint makes that adapter explicit and testable without requiring a
Rust toolchain or Wasmtime C SDK in the container.

## Validation

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime check`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime LIBFFI_LIBS= check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`

## Next Edge

Move from C scaffold wrappers to a real linker harness. The repo has Python
Wasmtime available, so a practical next slice is a Python memory64 fixture that
instantiates a tiny guest module importing the FFI/JIT wrapper-shaped functions
and verifies guest offsets and handle IDs across an actual Wasmtime `Memory64`.
