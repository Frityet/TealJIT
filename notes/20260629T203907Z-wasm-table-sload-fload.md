# 20260629T203907Z - WASM table SLOAD and FLOAD subset

Base commit: 0b6b0031

Added the first GC object lowering for WASM traces:

- guarded `IRT_TAB` SLOAD
- `FLOAD tab.asize`
- `FLOAD tab.array`
- `FLOAD tab.meta`

For GC64 table SLOADs, the trace builder now:

1. loads the raw 64-bit TValue
2. shifts right by 47 bits
3. compares against the table tag (`LJ_TTAB & 0x1ffff`, emitted as `131060`)
4. returns `0` on guard failure
5. masks the raw value with `LJ_GCVMASK` (`140737488355327`) to recover the
   object pointer

The synthetic trace fixture now emits table SLOAD/FLOAD instructions and
`wasm/tools` asserts `i64.and` appears in the lowered module. A disassembly check
showed the expected field offsets for the current GC64 layout:

- `tab.array`: 16
- `tab.meta`: 32
- `tab.asize`: 48

This is only the object/field layer. Table traces still need `ABC`, `AREF`,
`ALOAD`, `ASTORE`, `EQ NULL`, and real exit/snapshot handling before array
reads/writes can execute end to end.

Validation:

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `wasm-objdump -d /tmp/lj-wasm-ir-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
