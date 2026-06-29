/*
** WebAssembly VM bring-up surface.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_VM_WASM_H
#define _LJ_VM_WASM_H

#include "lj_obj.h"

typedef enum LJWasmVMSymbolKind {
  LJ_WASM_VM_FUNC,
  LJ_WASM_VM_DATA,
  LJ_WASM_VM_CONT,
  LJ_WASM_VM_FFI,
  LJ_WASM_VM_JIT,
  LJ_WASM_VM_HELPER
} LJWasmVMSymbolKind;

typedef struct LJWasmVMSymbol {
  const char *name;
  LJWasmVMSymbolKind kind;
  const char *summary;
} LJWasmVMSymbol;

LJ_FUNC const LJWasmVMSymbol *lj_vm_wasm_symbols(MSize *count);
LJ_FUNC const char *lj_vm_wasm_symbol_kind_name(LJWasmVMSymbolKind kind);

#endif
