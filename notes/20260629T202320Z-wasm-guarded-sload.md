# 20260629T202320Z - WASM guarded SLOAD lowering

Base commit: 081a60e2

Added the first GC64 typechecked SLOAD lowering for WASM traces. The trace
builder now accepts `IRSLOAD_TYPECHECK` for guarded `IRT_INT` and `IRT_NUM`
SLOADs while still rejecting `CONVERT`, `FRAME`, `PARENT`, and `KEYINDEX`.

Guarded SLOADs use one scratch `i64` local:

1. Load the raw 64-bit TValue from `base + 8 * (slot - 1 - LJ_FR2)`.
2. Shift the raw value right by 32 bits.
3. Compare against `(uint32_t)(LJ_TISNUM << 15)`, currently `4294508544`.
4. Return `0` on guard failure, matching the existing guard-exit placeholder.
5. Convert the payload:
   - int: `i32.wrap_i64`
   - num: `f64.reinterpret_i64`

The synthetic trace module now contains both read-only unguarded SLOADs and
guarded typechecked SLOADs. `wasm/tools` asserts the generated module includes
`i32.load`, `f64.load`, raw `i64.load`, `i64.shr_u`, `i64.eq`, `i64.lt_u`,
`i32.wrap_i64`, and `f64.reinterpret_i64`.

Remaining SLOAD work:

- `IRSLOAD_CONVERT` needs exact LuaJIT int/num conversion semantics.
- `IRSLOAD_FRAME` needs frame-link layout lowering.
- `PARENT`/side-trace inheritance and `KEYINDEX` still need dedicated rules.

Validation:

- `make -C src -j$(nproc)`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `git diff --check`
- `make -C wasm/tools clean && make -C wasm/tools check`
- `wasm-objdump -d /tmp/lj-wasm-ir-trace.wasm`
- `make -C wasm/host/wasmtime check`
- `src/luajit -e 'local ffi=require("ffi"); ffi.cdef[[int abs(int); typedef int (*cb_t)(int);]]; assert(ffi.C.abs(-5)==5); local cb=ffi.cast("cb_t", function(x) return x+1 end); assert(cb(6)==7); cb:free(); print("native smoke ok")'`
