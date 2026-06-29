# 2026-06-29T18:54:01Z subagent target survey

- Commit while surveyed: `9d145d2ca3db58493859c495489a0f08f627834f`
- Subagent: `019f14b6-0d2f-79c2-85b5-57394b385f24`
- Mode: read-only codebase survey; no files edited.

## Key checklist from survey

- `src/lj_arch.h`: add WASM64 architecture constants, detection, derived target
  macros, GC64, number mode, endian, page size, and initial feature gates.
- `src/lj_target.h` plus a new WASM target header: define allocator-visible
  target locations, register sets, spill slots, and `ExitState`.
- `src/vm_wasm64.dasc`: implement the VM entry points declared in
  `src/lj_vm.h`, including interpreter dispatch, hooks, exits, FFI call, and
  callback glue.
- `dynasm/dasm_wasm64.lua` and `.h`: add a WebAssembly DynASM backend or a
  replacement build path that can produce a wasm VM object/module.
- `src/host/buildvm.c` and `src/host/buildvm_asm.c`: teach buildvm how to use
  the new backend and emit the right artifact type.
- `src/lj_frame.h`: add `CFRAME_*` offsets and sizes matching the WASM VM.
- `src/lj_jit.h`, `src/lj_asm.c`, `src/lj_emit_wasm*.h`,
  `src/lj_asm_wasm*.h`: add the IR-to-Wasm lowering surface and patch/exit
  handling.
- `src/lj_ccall.h`, `src/lj_ccall.c`, `src/lj_ccallback.c`, and
  `src/lj_clib.c`: add a WASM FFI ABI. A host-mediated call path is expected
  because raw native function pointers are not generally callable from Wasm.
- `src/lj_mcode.c`: replace executable-memory assumptions with a WebAssembly
  module compile/instantiate/link contract.
- `src/jit/dis_wasm64.lua`, `src/jit/dump.lua`, and `src/jit/bcsave.lua`: add
  dump/disassembly/object-save support once the backend can emit code.

## Reusable staged gates

The survey called out existing staged-bring-up mechanisms:

- `LJ_ARCH_NOJIT`
- `LJ_OS_NOJIT`
- `LUAJIT_DISABLE_JIT`
- `LJ_ARCH_NOFFI`
- `LUAJIT_DISABLE_FFI`
- the missing-architecture callback fallback in `src/lj_ccallback.c`

This slice intentionally avoids those permanent disable gates for `wasm64`,
but the build remains gated until the VM/backend exists.
