# LuaJIT WASM64 Port Notes

This fork is bringing up WebAssembly support as a `wasm64` target. The target
policy is:

- WebAssembly memory64 only for the initial port.
- Little-endian linear memory.
- GC64 object references.
- 64 KiB page granularity.
- FFI and JIT are supported through explicit host imports, not by assuming
  native `dlopen`, arbitrary function pointer calls, or executable linear
  memory.
- WasmGC is optional and experimental.

## Current State

The repository has target metadata for `wasm64`, a WASM target ABI header, and a
host integration API in `src/lj_wasm_host.h`. The real VM/JIT backend is still
gated in `src/Makefile` until these pieces exist:

- `src/vm_wasm64.dasc` or an equivalent build path for the VM entry points.
- A WebAssembly encoder/buildvm path.
- `src/lj_emit_wasm*.h` and `src/lj_asm_wasm*.h` for trace IR lowering.
- Final `CFRAME_*` layout matched to the WASM VM.
- Full callback and exit-stub support.

Native builds continue to use the existing CPU backends.

## Host Contract

Core WebAssembly separates code from linear memory and does not let a module
write bytes into memory and jump to them as executable code. A full LuaJIT port
therefore needs host support.

The embedding host must provide imports compatible with `src/lj_wasm_host.h`:

- `lj_wasm_import_ffi_load`
- `lj_wasm_import_ffi_unload`
- `lj_wasm_import_ffi_symbol`
- `lj_wasm_import_ffi_call`
- `lj_wasm_import_jit_compile`
- `lj_wasm_import_jit_free`
- `lj_wasm_import_jit_enter`
- `lj_wasm_import_jit_patch_exit`

For FFI, the host resolves library and symbol handles and performs typed calls
from LuaJIT's prepared `CCallState`. This avoids relying on native function
pointer casts, which are not portable in WebAssembly because indirect calls are
signature-checked.

For JIT, the backend should emit WebAssembly modules/functions for traces. The
host compiles and instantiates those modules, returns opaque handles, enters
compiled traces through typed exports or table entries, and patches trace exits
through the host import.

## Suggested Hosts

- Wasmtime is a good server-side host because it supports memory64, function
  references, GC, tail calls, exception handling, precompilation, and caching.
- Emscripten can support browser/server builds with `-m64`/`MEMORY64`, side
  modules for dynamic linking, and JS table glue for function references.
- The WebAssembly component model is useful for exposing a typed Lua engine API,
  but it should not be treated as raw LuaJIT FFI.

## WasmGC

WasmGC is not a drop-in replacement for LuaJIT's existing GC or value layout.
The practical first use is likely host references or optional managed wrappers.
Only revisit object representation after the non-GC WASM64 runtime passes the
standard LuaJIT test surface.

## References

- WebAssembly JIT Interface explainer:
  <https://github.com/WebAssembly/jit-interface/blob/main/proposals/jit-interface/Explainer.md>
- Emscripten memory64:
  <https://emscripten.org/docs/tools_reference/settings_reference.html#memory64>
- Emscripten dynamic linking:
  <https://emscripten.org/docs/compiling/Dynamic-Linking.html>
- Emscripten function pointer issues:
  <https://emscripten.org/docs/porting/guidelines/function_pointer_issues.html>
- libffi WebAssembly support:
  <https://github.com/libffi/libffi>
- Wasmtime proposal support:
  <https://docs.wasmtime.dev/stability-wasm-proposals.html>
- Wasmtime precompilation:
  <https://docs.wasmtime.dev/examples-pre-compiling-wasm.html>
- Component model background:
  <https://component-model.bytecodealliance.org/design/why-component-model.html>
- WebAssembly features:
  <https://webassembly.org/features/>
