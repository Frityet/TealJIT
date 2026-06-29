# 20260629T205000Z - WASM array-read trace fixture and MOD

Base commit: 8b536fb1

Added integer `IR_MOD` lowering for WASM traces, emitted as `i32.rem_s`.

Added `wasm_trace_array_read_module`, a developer fixture for the native IR
shape of:

```
local t = {1, 2, 3}
local x = 0
for i = 1, 100 do
  x = x + t[(i % 3) + 1]
end
```

The fixture validates the current table-array read subset:

- converted loop-index SLOAD
- integer modulo and add for the array index
- guarded table SLOAD
- `tab.asize` and `tab.array` FLOADs
- `ABC` bounds guards
- `AREF` address calculations
- guarded numeric `ALOAD`
- guarded numeric accumulator SLOAD
- numeric accumulation and loop PHIs

`wasm/tools check` now validates the array-read module with memory64 enabled and
asserts `i32.rem_s`, `f64.reinterpret_i64`, `i64.lt_u`, `i32.ge_u`, and the loop
limit constant appear in the generated Wasm.

Remaining execution blockers are still host memory instantiation and real
side-exit/snapshot handling. The fixture proves the current lowering subset can
produce a valid module for this common array-read trace shape.

Validation:

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `wasm-objdump -d /tmp/lj-wasm-array-read-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
