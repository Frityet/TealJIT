# 20260629T195048Z - first WASM IR lowering scaffold

Base commit: `fd996a67`

## Slice

- Added the first trace-specific WebAssembly module builder in `src/lj_wasm_jit.c`.
- The builder walks `GCtrace` IR and lowers a deliberately small subset:
  `SLOAD`, `ADD`, `SUB`, `MUL`, `LE`, `LOOP`, and `PHI` for simple `int`/`num`
  shapes.
- Unsupported traces fall back to the existing NYI module.
- `lj_asm_trace()` now calls `lj_wasm_jit_compile_trace()` so host compilation
  sees the lowered module when the whitelist accepts the trace.
- Expanded `src/lj_wasm_emit.h` with the arithmetic, compare, and control
  opcodes required by the scaffold.
- Added `wasm/tools/wasm_trace_ir_module.c`, a synthetic trace-module emitter
  for deterministic validation of the lowered path.
- Added `wasm-validate` to `wasm/tools check`; WABT was installed in the
  container for this validation.

## Important limitation

This is not yet executable LuaJIT trace semantics. `SLOAD` currently materializes
zero-valued typed locals, guards return `0`, and successful completion still
returns `LJ_WASM_JIT_STATUS_NYI`. The value of this slice is that the IR walker,
typed local allocation, Wasm body generation, host compile wrapper, and validator
gate are in place for the next semantic lowering steps.

## Validation

- `make -C src -j$(nproc)`
- `make -C wasm/tools check`
- `cc -std=c99 -DLUAJIT_DISABLE_JIT -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit_nojit.o`
- `emcc -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I src -c src/lj_wasm_jit.c -o /tmp/lj_wasm_jit.o`
- `emcc -sMEMORY64=1 -std=c99 -Wall -Wextra -Werror -I src -c src/lj_asm.c -o /tmp/lj_asm.o`
- `make -C wasm/host/wasmtime check`
- Native FFI/callback smoke with `src/luajit`
- `git diff --check`

## Sidecar notes

- `notes/20260629T194613Z-wasm-ir-subset.md` surveys the smallest useful IR subset.
- `notes/20260629T194505Z-wasm-entry-dispatch.md` surveys the eventual trace-entry caller for `lj_wasm_host_jit_enter()`.
