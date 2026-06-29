# WASM64 VM Bring-Up

LuaJIT's interpreter is not a portable C loop in this fork. Each supported CPU
has a hand-written `src/vm_<arch>.dasc` backend that defines the bytecode
dispatch loop, fast functions, continuations, C API entry points, FFI glue, and
trace exits.

For WASM64, the VM must be implemented before the target can run Lua code. The
current build intentionally stops at the `wasm64` backend gate until this exists.

## Required Surface

`src/lj_vm_wasm.c` contains a compiled manifest of VM symbols the WASM backend
must provide or intentionally route through C/host support:

- C API entries: `lj_vm_call`, `lj_vm_pcall`, `lj_vm_cpcall`, `lj_vm_resume`
- unwind entries: `lj_vm_unwind_c`, `lj_vm_unwind_ff`,
  `lj_vm_unwind_c_eh`, `lj_vm_unwind_ff_eh`
- dispatch helpers and hooks: `lj_vm_record`, `lj_vm_inshook`,
  `lj_vm_rethook`, `lj_vm_callhook`, `lj_vm_profhook`, `lj_vm_IITERN`
- trace exits: `lj_vm_exit_handler`, `lj_vm_exit_interp`
- FFI bridges: `lj_vm_ffi_call`, `lj_vm_ffi_callback`
- continuations: `lj_cont_cat`, `lj_cont_ra`, `lj_cont_nop`,
  `lj_cont_condt`, `lj_cont_condf`, `lj_cont_hook`, `lj_cont_stitch`
- number helpers: `lj_vm_num2int_check`, `lj_vm_num2i64`,
  `lj_vm_num2u64`, `lj_vm_tobit`
- bytecode offset base: `lj_vm_asm_begin`

Some math helpers can use C/libm on WASM (`floor`, `ceil`, `trunc`) instead of
assembly stubs.

## Backend Options

### DynASM-to-Wasm

Add `dynasm/dasm_wasm64.lua`, `dynasm/dasm_wasm64.h`, and
`src/vm_wasm64.dasc`, then teach `host/buildvm` to emit a WebAssembly object or
module artifact.

Pros:

- Keeps the same VM-generation shape as existing targets.
- Allows tight control over bytecode dispatch and fast-function lowering.

Cons:

- DynASM currently assumes native instruction streams and relocation models.
- WebAssembly structured control flow and typed validation do not map cleanly to
  arbitrary label-and-branch assembly text.

### Generated C/Wasm VM

Build a WASM-specific C VM entry layer that implements the same `lj_vm_*`
surface and compiles through Emscripten/Clang to WebAssembly.

Pros:

- Uses the existing C compiler to produce valid Wasm control flow.
- Easier to debug early interpreter correctness.

Cons:

- Requires carefully matching LuaJIT frame, continuation, hook, and error
  semantics that existing targets encode in assembly.
- The JIT trace exit path still needs a separate host-assisted Wasm design.

## Recommended Path

Start with a generated C/Wasm VM for interpreter correctness, while keeping the
binary emitter in `src/lj_wasm_emit.*` for trace modules and future low-level
backend work. Once bytecode execution and C API entry points pass tests, add
FFI callbacks and trace exits.

The first executable milestone should run with JIT disabled and FFI restricted
to host imports. Only after the interpreter passes the standard LuaJIT behavior
surface should the trace-to-Wasm JIT become the main focus.
