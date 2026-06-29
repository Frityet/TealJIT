# 20260629T195554Z - WASM trace entry helper contract

Base commit: `c8eb0f27`

## Slice

- Added `lj_vm_wasm_trace_enter(lua_State *L, TValue *base, TraceNo traceno)`.
- The helper is the C-level contract for the future WASM `BC_JLOOP` dispatch
  path. It looks up `GCtrace->wasmjit`, sets `J->L`, `L->base`, `jit_base`, and
  positive trace `vmstate`, calls `lj_wasm_host_jit_enter(handle, L, base, 0)`,
  then restores interpreter state after the host export returns.
- Non-WASM builds return the NYI status without depending on WASM host imports.
- Added the helper to the WASM VM symbol manifest and documented the contract in
  `doc/wasm_vm.md`.
- Added `wasm/tools/wasm_trace_enter_contract.c` so the tools check links the
  helper and verifies the native fallback status.

## Validation

- `make -C src -j$(nproc)`
- `make -C wasm/tools check`
- `emcc -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I src -c src/lj_vm_wasm.c -o /tmp/lj_vm_wasm.o`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_vm_wasm.c -o /tmp/lj_vm_wasm_nojit.o`
- `make -C wasm/host/wasmtime check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Next

- Define the WASM trace-exit bridge contract and the status/result values that
  distinguish fallback, side exit, and Lua errors.
- Wire this helper into the generated WASM VM when the bytecode dispatch backend
  exists.
