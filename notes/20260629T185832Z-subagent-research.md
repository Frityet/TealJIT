# 2026-06-29T18:58:32Z subagent research

- Commit while researched: `9d145d2ca3db58493859c495489a0f08f627834f`
- Subagent: `019f14b6-2057-7512-b67f-728cfbfa7dfa`
- Mode: read-only WebAssembly feasibility/prior-art research; no files edited.

## Bottom line

A classic LuaJIT model where the guest allocates executable memory and jumps to
generated native code is not supported by core WebAssembly. A viable port needs:

1. A WASM64 interpreter/runtime target.
2. FFI behind an explicit host/embedder contract.
3. A host-assisted trace-to-Wasm JIT that emits modules/functions, asks the host
   to compile/instantiate them, and enters them through typed function
   references/table indices.
4. Optional WasmGC experiments after correctness and the value/GC model are
   stable.

## Sources captured by the sidecar

- Core WebAssembly has no in-guest dynamic code generation. The JIT Interface
  proposal explores `func.new`, but it is not a deployed baseline:
  <https://github.com/WebAssembly/jit-interface/blob/main/proposals/jit-interface/Explainer.md>
- LuaJIT upstream does not list WebAssembly as a supported target:
  <https://luajit.org/status.html>
- Emscripten `MEMORY64` / `-m64` documents wasm64 support:
  <https://emscripten.org/docs/tools_reference/settings_reference.html#memory64>
- Emscripten dynamic linking is main-module/side-module based:
  <https://emscripten.org/docs/compiling/Dynamic-Linking.html>
- Emscripten documents strict Wasm function-pointer signatures and traps for
  mismatched casts:
  <https://emscripten.org/docs/porting/guidelines/function_pointer_issues.html>
- Emscripten `addFunction` requires signatures and a growable table:
  <https://emscripten.org/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#calling-javascript-functions-as-function-pointers-from-c>
- libffi lists WASM32/WASM64 Emscripten support and has a `src/wasm` backend:
  <https://github.com/libffi/libffi>
- Wasmtime support matrix includes memory64, function references, GC, tail
  calls, and exception handling:
  <https://docs.wasmtime.dev/stability-wasm-proposals.html>
- Wasmtime supports precompilation/caching flows useful for trace modules:
  <https://docs.wasmtime.dev/examples-pre-compiling-wasm.html>
- The component model is a typed API boundary, not a raw shared-memory FFI:
  <https://component-model.bytecodealliance.org/design/why-component-model.html>
- WebAssembly lists GC as a feature, and web.dev marked WasmGC/tail calls as
  Baseline Newly available on 2024-12-11:
  <https://webassembly.org/features/>
  <https://web.dev/blog/wasmgc-wasm-tail-call-optimizations-baseline>

## Recommended staged plan

- Document target policy first: `wasm64`, little-endian, GC64, host-assisted JIT,
  host-mediated FFI, optional WasmGC.
- Add architecture/build skeleton, while clearly gating real target builds until
  the VM/backend exists.
- Solve the VM before FFI/JIT. LuaJIT has no portable C VM fallback in this fork.
- Bring up interpreter-only WASM64 in Wasmtime/Emscripten, then re-enable FFI via
  host imports.
- Build JIT as trace-IR-to-Wasm module/function emission with host compile and
  enter imports.
- Treat WasmGC as an R&D path for host references or future managed-object
  experiments, not a drop-in replacement for LuaJIT GC.
