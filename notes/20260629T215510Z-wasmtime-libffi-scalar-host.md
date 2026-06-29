# 20260629T215510Z Wasmtime Libffi Scalar Host

Base commit: `2b970dbaa53077f1d3b302baa5358e7b39455911`

## What Changed

- Added `LJ_WASM_FFI_CALL_ABI_VERSION`, `ccall_size`, `func_offset`, and
  `reserved` fields to `LJWasmFFICall`, populated by the WASM `ccall_set_args()`
  path.
- Added optional libffi support to the Wasmtime C scaffold. When
  `pkg-config libffi` succeeds, the scaffold builds with
  `LJ_WASMTIME_ENABLE_LIBFFI=1`, links `libffi` plus configurable `DL_LIBS`,
  and provides default `dlopen`, `dlsym`, and fixed-arity numeric scalar
  `ffi_call` behavior.
- Implemented tagged opaque scaffold handles for libraries and symbols instead
  of passing raw `dlopen`/`dlsym` pointers through the API. Library unload
  invalidates dependent symbol handles.
- Allowed null-handle symbol lookup in libffi mode for `ffi.C`-style default
  namespace resolution via `RTLD_DEFAULT`.
- Tightened descriptor validation: ABI version, `ccall_size`, reserved field,
  known flags, `sig.rettype == ret.type`, offset bounds, unsupported/vararg
  rejection, and numeric-only slot support.
- Normalized libffi arguments/results into local storage before/after
  `ffi_call`, avoiding direct calls through arbitrary `CCallState` slot storage.
- Updated the Wasmtime README, pseudocode, and host contract tests. The libffi
  test path now loads `libm`, calls `cos(0.0)`, resolves default namespace
  `abs`, rejects unsupported/pointer/reserved descriptors, and rejects calls
  through a symbol invalidated by unload.

## Subagent Audit

Pauli (`019f1556-ac39-7f00-871a-5e952ccad8bc`) reviewed the intended libffi
slice before commit. The code in this checkpoint incorporates the high-priority
findings: default namespace handling, explicit callee offset, pointer rejection,
unsupported/vararg rejection, tagged handles, local value normalization, and
descriptor consistency checks. Full guest-memory translation remains a real
Wasmtime embedding task rather than a C scaffold shortcut.

## Validation

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime check`
- `make -C wasm/host/wasmtime clean && make -C wasm/host/wasmtime LIBFFI_LIBS= check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
- `git diff --check`

## Next Edge

Bridge this host-side scaffold into an actual Wasmtime linker that translates
guest WASM64 pointers to checked native views. Numeric scalar calls now have a
host ABI and testable execution path; pointer arguments, aggregates, varargs,
callbacks, and true guest memory access should stay rejected until that memory
translation layer exists.
