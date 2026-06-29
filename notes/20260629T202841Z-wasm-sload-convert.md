# 20260629T202841Z - WASM unguarded SLOAD conversion

Base commit: 2b17ce32

Added unguarded `IRSLOAD_CONVERT` lowering for integer/number stack payloads.
This covers the simple numeric-loop shape observed in native dumps where the
loop index is loaded with `SLOAD #3 CI` and converted from a numeric slot into
an integer result.

Current lowering:

- result `IRT_INT`: `f64.load` followed by `i32.trunc_f64_s`
- result `IRT_NUM`: `i32.load` followed by `f64.convert_i32_s`

The trace whitelist still rejects `IRSLOAD_CONVERT | IRSLOAD_TYPECHECK`.
Native backends use additional guarded conversion paths there, including exact
integer conversion checks. That needs a separate implementation so Wasm traps or
out-of-range conversion behavior do not become observable in cases where LuaJIT
expects a side exit.

The synthetic trace module now includes both unguarded conversion directions and
`wasm/tools` asserts `i32.trunc_f64_s` and `f64.convert_i32_s` are emitted.

Validation:

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `wasm-objdump -d /tmp/lj-wasm-ir-trace.wasm | grep -n -E 'i32.trunc_f64_s|f64.convert_i32_s'`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
