//! Documentation-only sketch for wiring the LuaJIT WASM64 imports into
//! Wasmtime. This file is not built by this scaffold.
//!
//! The C ABI in `include/lj_wasmtime_host.h` names native pointer parameters,
//! but a real Wasmtime host receives guest pointers as `i64` values for a
//! WASM64 guest. Every pointer below must be bounds-checked against guest memory
//! before reading or writing.

use wasmtime::{Caller, Linker};

const OK: i32 = 0;
const ERR: i32 = -1;
const NYI: i32 = -2;

struct HostState {
    handles: HandleTable,
}

struct HandleTable;

impl HandleTable {
    fn insert_todo(&mut self) -> u64 {
        0
    }
}

fn register_luajit_host_imports(linker: &mut Linker<HostState>) -> anyhow::Result<()> {
    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_load",
        |mut caller: Caller<'_, HostState>,
         name_ptr: i64,
         global: i32,
         handle_out_ptr: i64|
         -> i32 {
            let _ = (caller, name_ptr, global, handle_out_ptr);
            // TODO:
            // 1. Read a NUL-terminated library name from guest memory.
            // 2. Resolve either a named library or the global namespace.
            // 3. Store an opaque handle-table ID into `handle_out_ptr`.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_unload",
        |mut caller: Caller<'_, HostState>, handle: i64| {
            let _ = (caller, handle);
            // TODO: remove the handle-table entry and release the library.
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_symbol",
        |mut caller: Caller<'_, HostState>,
         handle: i64,
         name_ptr: i64,
         symbol_out_ptr: i64|
         -> i32 {
            let _ = (caller, handle, name_ptr, symbol_out_ptr);
            // TODO: resolve a symbol and write an opaque symbol handle.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_call",
        |mut caller: Caller<'_, HostState>, cts: i64, ct: i64, cc: i64| -> i32 {
            let _ = (caller, cts, ct, cc);
            // TODO: decode LuaJIT FFI call state from guest memory and marshal
            // arguments/results across the host boundary.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_callback_new",
        |mut caller: Caller<'_, HostState>,
         slot: i32,
         ctypeid: i32,
         handle_out_ptr: i64|
         -> i32 {
            let _ = (caller, slot, ctypeid, handle_out_ptr);
            // TODO: create a callable table/host function handle that knows
            // how to enter the guest callback slot.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_callback_slot",
        |mut caller: Caller<'_, HostState>, handle: i64, slot_out_ptr: i64| -> i32 {
            let _ = (caller, handle, slot_out_ptr);
            // TODO: look up the callback handle and write its LuaJIT slot.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_callback_free",
        |mut caller: Caller<'_, HostState>, handle: i64| {
            let _ = (caller, handle);
            // TODO: release the callback table/host function handle.
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_compile",
        |mut caller: Caller<'_, HostState>, module_ptr: i64, handle_out_ptr: i64| -> i32 {
            let _ = (caller, module_ptr, handle_out_ptr);
            // TODO: read LJWasmJITModule, copy module bytes out of guest
            // memory, compile them with Wasmtime, then store a trace handle.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_free",
        |mut caller: Caller<'_, HostState>, handle: i64| {
            let _ = (caller, handle);
            // TODO: drop the compiled trace handle.
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_enter",
        |mut caller: Caller<'_, HostState>,
         handle: i64,
         lua_state: i64,
         base: i64,
         exitno: i32|
         -> i32 {
            let _ = (caller, handle, lua_state, base, exitno);
            // TODO: call the compiled trace entry export and translate traps
            // into LuaJIT host status codes.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_patch_exit",
        |mut caller: Caller<'_, HostState>, from: i64, exitno: i32, to: i64| -> i32 {
            let _ = (caller, from, exitno, to);
            // TODO: record an exit continuation from one trace handle to
            // another. The guest-visible handles must remain opaque.
            NYI
        },
    )?;

    Ok(())
}
