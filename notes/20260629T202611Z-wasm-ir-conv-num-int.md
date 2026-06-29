# 20260629T202611Z - WASM IR_CONV num.int lowering

Base commit: 9ad35227

Added lowering for `IR_CONV` with `IRCONV_NUM_INT`, emitted as:

```
local.get <i32 source>
f64.convert_i32_s
local.set <f64 destination>
```

This is one of the next blockers seen in a native dump of a plain numeric loop:

```
int SLOAD #3 CI
num SLOAD #2 T
num CONV 0001 num.int
num ADD ...
```

The `IRSLOAD_CONVERT` part of that loop index load is still unsupported and
needs a separate lowering because it combines slot loading with LuaJIT's type
guard/conversion semantics. This commit only handles explicit `IR_CONV
num.int` nodes after an integer value is already available.

Validation:

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `wasm-objdump -d /tmp/lj-wasm-ir-trace.wasm | grep -n 'f64.convert_i32_s'`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
