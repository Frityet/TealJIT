# 20260629T204711Z - WASM array-store trace fixture

Base commit: 710f605f

Added `wasm_trace_array_store_module`, a developer fixture for the native IR
shape of:

```
local t = {}
for i = 1, 100 do t[i] = i end
```

The fixture exercises the current table-array lowering subset end to end:

- converted numeric loop index SLOAD
- guarded table SLOAD
- `tab.asize`, `tab.array`, and `tab.meta` FLOADs
- `ABC` bounds guards
- `AREF` address calculations
- `EQ NULL` metatable guard
- `IR_CONV num.int`
- numeric `ASTORE`
- loop increment/limit guards and PHI

`wasm/tools check` now emits and validates this module with memory64 enabled,
then asserts the generated Wasm contains the expected conversion, store, GC64
table-mask, bounds, and constant opcodes.

This remains a lowering/validation fixture. Real execution still needs the
Wasmtime memory import hookup and real side-exit/snapshot restoration.

Validation:

- `make -C wasm/tools clean && make -C wasm/tools check`
- `git diff --check`
- `wasm-objdump -d /tmp/lj-wasm-array-store-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
