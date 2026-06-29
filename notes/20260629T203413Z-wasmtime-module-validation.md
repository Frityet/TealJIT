# 20260629T203413Z - Wasmtime JIT module validation

Base commit: c628059f

Added `lj_wasmtime_host_validate_jit_module()` to the Wasmtime C scaffold and
made `lj_wasm_import_jit_compile()` call it before delegating to the installed
compile hook.

The validator rejects:

- null/empty module byte ranges
- unknown JIT flags
- unknown memory flags
- non-zero reserved fields
- memory descriptors without `LJ_WASM_JIT_F_IMPORT_ENV_MEMORY`
- imported memories that are not marked memory64
- max-page descriptors smaller than min-page descriptors
- max-page values without `LJ_WASM_JIT_MEMORY_F_HAS_MAX`

The contract test now checks valid lowered modules, valid non-memory modules,
and several invalid memory-descriptor cases. The intent is to keep a future real
Wasmtime compile hook from seeing malformed trace metadata or instantiating a
memory-importing trace against the wrong host setup.

Validation:

- `make -C wasm/host/wasmtime check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
- `git diff --check`
