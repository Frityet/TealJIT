# 20260629T203103Z - WASM numeric loop trace fixture

Base commit: 8bd2c98e

Added `wasm_trace_numeric_for_module`, a developer tool that builds the IR shape
from a native dump of:

```
local x = 0
for i = 1, 100 do x = x + i end
```

The fixture includes:

- loop-index `SLOAD #3` with `IRSLOAD_CONVERT|IRSLOAD_INHERIT`
- accumulator `SLOAD #2` with `IRSLOAD_TYPECHECK`
- `IR_CONV num.int`
- integer constants below `REF_BASE`
- integer and number `ADD`
- integer `LE` guards
- loop `PHI`s

`wasm/tools check` now validates this module with memory64 enabled and asserts
the lowered module imports `env.memory` and contains the expected conversion,
typecheck, and constant opcodes. This gives us a concrete regression signal for
the current plain numeric for-loop subset, even though full trace execution is
still blocked on host memory instantiation and real side-exit restoration.

Validation:

- `make -C wasm/tools clean && make -C wasm/tools check`
- `git diff --check`
- `wasm-objdump -d /tmp/lj-wasm-numeric-for-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
