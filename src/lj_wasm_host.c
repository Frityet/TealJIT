/*
** WebAssembly host integration API.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_wasm_host_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_wasm_host.h"

#if LJ_TARGET_WASM

extern int lj_wasm_import_ffi_load(const char *name, int global,
				   LJWasmHostHandle *handle);
extern void lj_wasm_import_ffi_unload(LJWasmHostHandle handle);
extern int lj_wasm_import_ffi_symbol(LJWasmHostHandle handle, const char *name,
				     LJWasmHostHandle *symbol);
extern int lj_wasm_import_ffi_call(struct CTState *cts, struct CType *ct,
				   struct CCallState *cc,
				   const LJWasmFFICall *call);
extern int lj_wasm_import_ffi_callback_new(uint32_t slot, uint32_t ctypeid,
					   LJWasmHostHandle *handle);
extern int lj_wasm_import_ffi_callback_slot(LJWasmHostHandle handle,
					    uint32_t *slot);
extern void lj_wasm_import_ffi_callback_free(LJWasmHostHandle handle);

extern int lj_wasm_import_jit_compile(const LJWasmJITModule *module,
				      LJWasmHostHandle *handle);
extern void lj_wasm_import_jit_free(LJWasmHostHandle handle);
extern int lj_wasm_import_jit_enter(LJWasmHostHandle handle, void *lua_state,
				    void *base, void *exit_state,
				    uint32_t exitno);
extern int lj_wasm_import_jit_patch_exit(LJWasmHostHandle from,
					 uint32_t exitno,
					 LJWasmHostHandle to);

int lj_wasm_host_ffi_load(const char *name, int global,
			  LJWasmHostHandle *handle)
{
  return lj_wasm_import_ffi_load(name, global, handle);
}

void lj_wasm_host_ffi_unload(LJWasmHostHandle handle)
{
  lj_wasm_import_ffi_unload(handle);
}

int lj_wasm_host_ffi_symbol(LJWasmHostHandle handle, const char *name,
			    LJWasmHostHandle *symbol)
{
  return lj_wasm_import_ffi_symbol(handle, name, symbol);
}

int lj_wasm_host_ffi_call(struct CTState *cts, struct CType *ct,
			  struct CCallState *cc, const LJWasmFFICall *call)
{
  return lj_wasm_import_ffi_call(cts, ct, cc, call);
}

int lj_wasm_host_ffi_callback_new(uint32_t slot, uint32_t ctypeid,
				  LJWasmHostHandle *handle)
{
  return lj_wasm_import_ffi_callback_new(slot, ctypeid, handle);
}

int lj_wasm_host_ffi_callback_slot(LJWasmHostHandle handle, uint32_t *slot)
{
  return lj_wasm_import_ffi_callback_slot(handle, slot);
}

void lj_wasm_host_ffi_callback_free(LJWasmHostHandle handle)
{
  lj_wasm_import_ffi_callback_free(handle);
}

int lj_wasm_host_jit_compile(const LJWasmJITModule *module,
			     LJWasmHostHandle *handle)
{
  return lj_wasm_import_jit_compile(module, handle);
}

void lj_wasm_host_jit_free(LJWasmHostHandle handle)
{
  lj_wasm_import_jit_free(handle);
}

int lj_wasm_host_jit_enter(LJWasmHostHandle handle, void *lua_state,
			   void *base, void *exit_state, uint32_t exitno)
{
  return lj_wasm_import_jit_enter(handle, lua_state, base, exit_state, exitno);
}

int lj_wasm_host_jit_patch_exit(LJWasmHostHandle from, uint32_t exitno,
				LJWasmHostHandle to)
{
  return lj_wasm_import_jit_patch_exit(from, exitno, to);
}

#else

int lj_wasm_host_c_dummy;

#endif
