# 20260629T204452Z - WASM ALOAD, ASTORE, and EQ NULL

Base commit: 4f92eb3e

Added the first typed array slot read/write lowering for WASM traces:

- `IR_ALOAD` for int/num TValue reads from an `AREF` address
- `IR_ASTORE` for numeric TValue writes
- `IR_EQ` guards for integer/pointer equality, including `tab.meta == NULL`

Guarded numeric `ALOAD` mirrors guarded SLOAD:

1. load the raw 64-bit TValue from the AREF address
2. check the high word against `LJ_TISNUM << 15`
3. return `0` on guard failure
4. reinterpret the payload as `f64`

Numeric `ASTORE` currently emits `f64.store` only. That covers numeric array
slots, but integer/GC-object stores still need explicit TValue construction.

The synthetic trace fixture now includes table SLOAD/FLOAD, `ABC`, `AREF`,
guarded numeric `ALOAD`, numeric `ASTORE`, and `EQ NULL` for `tab.meta`.

Still missing for real table traces:

- integer and GC-object ASTORE/ALOAD variants
- hash/HREF paths
- metamethod exits beyond the placeholder `return 0`
- real side-exit snapshot restoration

Validation:

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `wasm-objdump -d /tmp/lj-wasm-ir-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
