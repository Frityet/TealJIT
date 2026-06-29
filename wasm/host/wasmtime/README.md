# LuaJIT WASM64 Wasmtime Host Scaffold

This directory is a concrete host-side scaffold for the imports declared in
`src/lj_wasm_host.h`. It is intentionally self-contained so it can be reviewed
and syntax-checked without a local Rust toolchain or a Wasmtime C library.

The C shim exports the exact raw import symbol names expected by the guest.
LuaJIT's in-guest wrappers are named `lj_wasm_host_*`; they call host imports
named `lj_wasm_import_*`.

| Guest import | Scaffold behavior | Real Wasmtime responsibility |
| --- | --- | --- |
| `lj_wasm_import_ffi_load` | Validates output pointer, clears it, delegates to a hook, otherwise returns `LJ_WASM_HOST_NYI`. | Read the guest C string, load or resolve a library handle, store an opaque handle. |
| `lj_wasm_import_ffi_unload` | Delegates to a hook when present. | Release the host library/symbol-table handle. |
| `lj_wasm_import_ffi_symbol` | Validates inputs, clears output, delegates, otherwise returns `NYI`. | Read the guest C string and resolve a symbol from a loaded handle. |
| `lj_wasm_import_ffi_call` | Validates opaque pointers, delegates, otherwise returns `NYI`. | Marshal `CTState`, `CType`, and `CCallState` from guest memory into a safe native call. |
| `lj_wasm_import_ffi_callback_new` | Clears output, delegates, otherwise returns `NYI`. | Allocate a callable Wasm table/host function handle for a LuaJIT callback slot. |
| `lj_wasm_import_ffi_callback_slot` | Validates handle/output, delegates, otherwise returns `NYI`. | Map a callback handle back to its LuaJIT callback slot for `callback:set/free`. |
| `lj_wasm_import_ffi_callback_free` | Delegates to a hook when present. | Release a callback table/host function handle. |
| `lj_wasm_import_jit_compile` | Validates module/output, clears output, delegates, otherwise returns `NYI`. | Decode `LJWasmJITModule`, compile the trace module with Wasmtime, and store a compiled trace handle. |
| `lj_wasm_import_jit_free` | Delegates to a hook when present. | Drop a compiled trace handle. |
| `lj_wasm_import_jit_enter` | Validates handle, delegates, otherwise returns `NYI`. | Enter a compiled trace export with guest state/base/exit number. |
| `lj_wasm_import_jit_patch_exit` | Validates handles, delegates, otherwise returns `NYI`. | Patch a trace exit continuation from one compiled trace to another. |

## Files

- `include/lj_wasmtime_host.h` mirrors the import ABI and defines a hook table,
  import-name constants, status values, and import metadata for linker setup.
- `src/lj_wasmtime_host.c` provides default stub implementations for all eight
  `lj_wasm_import_*` imports. Embedders can install hooks with
  `lj_wasmtime_host_set_hooks`.
- `examples/wasmtime_linker_pseudocode.rs` sketches how a Rust Wasmtime host
  would register the imports. It is documentation only and is not built here.
- `Makefile` provides a local syntax check using the available C compiler.

## Wasmtime Mapping Notes

Most values that look like pointers in `src/lj_wasm_host.h` are guest pointers.
A Wasmtime embedding must treat them as `i64` offsets for a WASM64 guest,
validate them against the guest memory, and only then read or write guest data.
Do not cast guest pointer values directly to native pointers.

Host handles should be opaque guest-visible IDs or table indexes rather than raw
native pointers. The guest stores them in pointer-sized slots, but the host owns
the backing objects and lifetime.

The C scaffold uses direct native pointer types because it is an ABI contract
and test stub. The Rust pseudocode shows the memory-translation boundary that a
real Wasmtime linker needs.

## Validation

Run:

```sh
make -C wasm/host/wasmtime check
```

This checks that the scaffold is valid C without requiring Rust, Cargo, or a
Wasmtime SDK.
