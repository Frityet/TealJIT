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
            // 1. Borrow guest memory into LJWasmtimeGuestMemory.
            // 2. Call lj_wasmtime_guest_ffi_load with the per-instance guest
            //    context so the helper reads the name and writes the handle ID.
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
            // TODO: call lj_wasmtime_guest_ffi_symbol with the per-instance
            // guest context so the helper reads the name and writes the symbol
            // handle ID.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_ffi_call",
        |mut caller: Caller<'_, HostState>, cts: i64, ct: i64, cc: i64, call: i64| -> i32 {
            let _ = (caller, cts, ct, cc, call);
            // TODO: call lj_wasmtime_guest_ffi_call with the per-instance guest
            // context. It copies the guest CCallState, substitutes the native
            // symbol handle in the temporary copy, calls the scalar backend, and
            // writes the updated frame back with the guest handle ID restored.
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
            // TODO: call lj_wasmtime_guest_jit_compile with the per-instance
            // guest context. The helper reads LJWasmJITModule, copies module
            // bytes out of guest memory for the raw compile import, and writes
            // the compiled trace handle ID back to the guest.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_free",
        |mut caller: Caller<'_, HostState>, handle: i64| {
            let _ = (caller, handle);
            // TODO: call lj_wasmtime_guest_jit_free with the per-instance
            // guest context.
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_enter",
        |mut caller: Caller<'_, HostState>,
         handle: i64,
         lua_state: i64,
         base: i64,
         exit_state: i64,
         exitno: i32|
         -> i32 {
            let _ = (caller, handle, lua_state, base, exit_state, exitno);
            // TODO: call lj_wasmtime_guest_jit_enter with the per-instance
            // guest context and return the trace status.
            NYI
        },
    )?;

    linker.func_wrap(
        "env",
        "lj_wasm_import_jit_patch_exit",
        |mut caller: Caller<'_, HostState>, from: i64, exitno: i32, to: i64| -> i32 {
            let _ = (caller, from, exitno, to);
            // TODO: call lj_wasmtime_guest_jit_patch_exit with the
            // per-instance guest context.
            NYI
        },
    )?;

    Ok(())
}
