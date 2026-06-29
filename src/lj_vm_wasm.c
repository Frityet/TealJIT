/*
** WebAssembly VM bring-up surface.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_vm_wasm_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_vm_wasm.h"

static const LJWasmVMSymbol wasm_vm_symbols[] = {
  { "lj_vm_asm_begin", LJ_WASM_VM_DATA,
    "Base address for bytecode dispatch offsets." },
  { "lj_vm_call", LJ_WASM_VM_FUNC,
    "Enter Lua code from the C API." },
  { "lj_vm_pcall", LJ_WASM_VM_FUNC,
    "Protected Lua call entry." },
  { "lj_vm_cpcall", LJ_WASM_VM_FUNC,
    "Protected C callback entry." },
  { "lj_vm_resume", LJ_WASM_VM_FUNC,
    "Coroutine resume entry." },
  { "lj_vm_unwind_c", LJ_WASM_VM_FUNC,
    "Unwind from a C frame." },
  { "lj_vm_unwind_ff", LJ_WASM_VM_FUNC,
    "Unwind from a fast-function frame." },
  { "lj_vm_unwind_c_eh", LJ_WASM_VM_FUNC,
    "Exception-handling landing pad for C-frame unwinds." },
  { "lj_vm_unwind_ff_eh", LJ_WASM_VM_FUNC,
    "Exception-handling landing pad for fast-function unwinds." },
  { "lj_vm_record", LJ_WASM_VM_JIT,
    "Recorder dispatch hook from the interpreter." },
  { "lj_vm_inshook", LJ_WASM_VM_FUNC,
    "Instruction hook dispatch." },
  { "lj_vm_rethook", LJ_WASM_VM_FUNC,
    "Return hook dispatch." },
  { "lj_vm_callhook", LJ_WASM_VM_FUNC,
    "Call hook dispatch." },
  { "lj_vm_profhook", LJ_WASM_VM_FUNC,
    "Profiler hook dispatch." },
  { "lj_vm_IITERN", LJ_WASM_VM_FUNC,
    "Iterator helper for bytecode dispatch." },
  { "lj_vm_exit_handler", LJ_WASM_VM_JIT,
    "Trace exit handler entry." },
  { "lj_vm_exit_interp", LJ_WASM_VM_JIT,
    "Trace exit-to-interpreter entry." },
  { "lj_vm_ffi_call", LJ_WASM_VM_FFI,
    "Low-level FFI call bridge; WASM routes through host imports." },
  { "lj_vm_ffi_callback", LJ_WASM_VM_FFI,
    "Low-level FFI callback bridge; WASM needs table/host callback support." },
  { "lj_cont_cat", LJ_WASM_VM_CONT,
    "Continuation for concatenation metamethods." },
  { "lj_cont_ra", LJ_WASM_VM_CONT,
    "Continuation storing a result in bytecode RA." },
  { "lj_cont_nop", LJ_WASM_VM_CONT,
    "No-op continuation." },
  { "lj_cont_condt", LJ_WASM_VM_CONT,
    "Truthy conditional continuation." },
  { "lj_cont_condf", LJ_WASM_VM_CONT,
    "Falsy conditional continuation." },
  { "lj_cont_hook", LJ_WASM_VM_CONT,
    "Hook-yield continuation." },
  { "lj_cont_stitch", LJ_WASM_VM_CONT,
    "Trace stitching continuation." },
  { "lj_vm_num2int_check", LJ_WASM_VM_HELPER,
    "Checked number-to-int32 conversion." },
  { "lj_vm_num2i64", LJ_WASM_VM_HELPER,
    "Number-to-int64 conversion with LuaJIT semantics." },
  { "lj_vm_num2u64", LJ_WASM_VM_HELPER,
    "Number-to-uint64 conversion with LuaJIT semantics." },
  { "lj_vm_tobit", LJ_WASM_VM_HELPER,
    "Lua BitOp number conversion." },
  { "lj_vm_next", LJ_WASM_VM_JIT,
    "JIT helper for table iteration." }
};

const LJWasmVMSymbol *lj_vm_wasm_symbols(MSize *count)
{
  if (count)
    *count = (MSize)(sizeof(wasm_vm_symbols) / sizeof(wasm_vm_symbols[0]));
  return wasm_vm_symbols;
}

const char *lj_vm_wasm_symbol_kind_name(LJWasmVMSymbolKind kind)
{
  switch (kind) {
  case LJ_WASM_VM_FUNC: return "func";
  case LJ_WASM_VM_DATA: return "data";
  case LJ_WASM_VM_CONT: return "continuation";
  case LJ_WASM_VM_FFI: return "ffi";
  case LJ_WASM_VM_JIT: return "jit";
  case LJ_WASM_VM_HELPER: return "helper";
  default: return "unknown";
  }
}
