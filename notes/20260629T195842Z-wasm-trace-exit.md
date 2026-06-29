# 20260629T195842Z - WASM trace exit helper contract

Base commit: `97a77e2e`

## Slice

- Added `lj_vm_wasm_trace_exit(lua_State *L, TraceNo parent, ExitNo exitno,
  ExitState *ex)`.
- The helper validates the explicit parent trace and exit number, sets
  `J->L`, `J->parent`, and `J->exitno`, then calls `lj_trace_exit(J, ex)`.
- This is the matching C-level contract for a future host/WASM trace-exit bridge.
  WASM cannot recover the parent trace from a native PC register, so the bridge
  must pass parent/exit explicitly.
- Extended the VM symbol manifest and `doc/wasm_vm.md` with the exit contract.
- Extended `wasm_trace_enter_contract` so `wasm/tools check` verifies both entry
  and exit native fallback statuses.

## Validation

- `make -C src -j$(nproc)`
- `emcc -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I src -c src/lj_vm_wasm.c -o /tmp/lj_vm_wasm.o`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_vm_wasm.c -o /tmp/lj_vm_wasm_nojit.o`
- `make -C wasm/tools check`
- `make -C wasm/host/wasmtime check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Next

- Define the concrete return/status ABI between Wasm trace exports, the WASM VM,
  and the host when a trace exits or falls back.
- Fill `ExitState` from emitted trace locals/spills before calling this helper.
