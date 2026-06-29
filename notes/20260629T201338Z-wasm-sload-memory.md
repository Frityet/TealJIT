# 20260629T201338Z - WASM SLOAD memory lowering

Base commit: 69c2ca4f

Implemented the first memory-backed WASM trace loads. Lowered trace modules now
import `env.memory` as a memory64 memory when the emitted trace body needs stack
slot payload loads. The current entry signature still takes the Lua stack base as
parameter 1; raw SLOAD addresses are emitted as:

```
base + 8 * (slot - 1 - LJ_FR2)
```

For WASM64/GC64, `LJ_FR2 == 1`, matching the native backend's stack slot
payload offset rule. The emitter now has memory import, memory argument, i32.load,
i64.load, f64.load, and i64.add primitives. The synthetic trace IR test emits
read-only int/num SLOADs and validates to a module with:

```
Import[1]:
 - memory[0] pages: initial=1 i64 <- env.memory
...
i32.load 2 0
f64.load 3 0
```

The lowering deliberately rejects guarded/typechecked/converted SLOAD modes for
now (`PARENT`, `FRAME`, `TYPECHECK`, `CONVERT`, and `KEYINDEX`). Real Lua traces
that require type checks or conversions still fall back to the NYI module path.
The next useful step is to lower the GC64 tag/type checks around SLOAD, then wire
host memory ownership so Wasmtime instances import the same linear memory as the
compiled Lua state.

Validation:

- `make -C src -j$(nproc)`
- `make -C wasm/tools check`
- `wasm-objdump -x/-d /tmp/lj-wasm-ir-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
