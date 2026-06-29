# LuaJIT WASM Support

This directory holds host-side support code and experiments for the `wasm64`
target.

The core runtime contract lives in `src/lj_wasm_host.h`. Host implementations
must provide the raw `lj_wasm_import_*` imports used by the in-guest
`lj_wasm_host_*` wrappers:

- FFI library loading and symbol lookup.
- Typed FFI calls from LuaJIT's prepared `CCallState`.
- Trace module compilation/instantiation.
- Trace entry and exit patching.

## Toolchains

The initial target is WASM64:

```sh
emcc -sMEMORY64=1
clang --target=wasm64-unknown-unknown
```

The top-level LuaJIT build currently detects `wasm64` and stops at an explicit
backend gate. That is intentional until the VM and IR-to-Wasm backend are added.

## Host Layout

- `host/wasmtime/`: Wasmtime-oriented import scaffold.
- Future host directories may cover Emscripten/browser table glue, WASI preview
  integrations, or component-model wrappers.
