/*
** WebAssembly host integration API.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_WASM_HOST_H
#define _LJ_WASM_HOST_H

#include "lj_def.h"

struct CCallState;
struct CTState;
struct CType;

#if LJ_TARGET_WASM

/* Host symbol handles are stored in FFI cdata pointer slots. */
typedef void *LJWasmHostHandle;

typedef enum LJWasmHostStatus {
  LJ_WASM_HOST_OK = 0,
  LJ_WASM_HOST_ERR = -1,
  LJ_WASM_HOST_NYI = -2
} LJWasmHostStatus;

typedef enum LJWasmScalarType {
  LJ_WASM_SCALAR_VOID,
  LJ_WASM_SCALAR_I32,
  LJ_WASM_SCALAR_I64,
  LJ_WASM_SCALAR_F32,
  LJ_WASM_SCALAR_F64,
  LJ_WASM_SCALAR_PTR,
  LJ_WASM_SCALAR_AGG
} LJWasmScalarType;

#define LJ_WASM_FFI_MAX_ARGS		16u

typedef enum LJWasmFFILoc {
  LJ_WASM_FFI_LOC_NONE,
  LJ_WASM_FFI_LOC_GPR,
  LJ_WASM_FFI_LOC_FPR,
  LJ_WASM_FFI_LOC_STACK,
  LJ_WASM_FFI_LOC_RETREF
} LJWasmFFILoc;

#define LJ_WASM_FFI_SIG_F_VARARG	0x01u
#define LJ_WASM_FFI_SIG_F_UNSUPPORTED	0x80u

#define LJ_WASM_FFI_SLOT_F_UNSIGNED	0x01u

typedef struct LJWasmFFISig {
  uint32_t ctypeid;		/* LuaJIT CTypeID of the function. */
  uint16_t nargs;		/* Number of populated argument slots. */
  uint8_t flags;		/* Vararg/calling-convention flags. */
  uint8_t rettype;		/* LJWasmScalarType. */
} LJWasmFFISig;

typedef struct LJWasmFFISlot {
  uint8_t type;			/* LJWasmScalarType. */
  uint8_t loc;			/* LJWasmFFILoc in CCallState. */
  uint16_t flags;		/* LJ_WASM_FFI_SLOT_F_* flags. */
  uint32_t offset;		/* Byte offset from CCallState*. */
  uint32_t size;		/* Original CType storage size. */
} LJWasmFFISlot;

typedef struct LJWasmFFICall {
  LJWasmFFISig sig;
  LJWasmFFISlot ret;
  LJWasmFFISlot args[LJ_WASM_FFI_MAX_ARGS];
} LJWasmFFICall;

#define LJ_WASM_JIT_F_IR_LOWERED	0x00000001u
#define LJ_WASM_JIT_F_IMPORT_ENV_MEMORY	0x00000002u

#define LJ_WASM_JIT_MEMORY_F_64		0x00000001u
#define LJ_WASM_JIT_MEMORY_F_HAS_MAX	0x00000002u

typedef struct LJWasmJITModule {
  const uint8_t *bytes;		/* Encoded Wasm module bytes. */
  size_t size;			/* Size of bytes. */
  uint32_t trace;		/* LuaJIT trace number. */
  uint32_t entry;		/* Entry export/function index. */
  uint32_t exit;		/* Exit trampoline export/function index. */
  uint32_t flags;		/* Backend feature flags. */
  uint64_t memory_min;		/* Required imported memory minimum pages. */
  uint64_t memory_max;		/* Required imported memory maximum pages. */
  uint32_t memory_flags;	/* LJ_WASM_JIT_MEMORY_F_* for env.memory. */
  uint32_t reserved;		/* Reserved for ABI-compatible extensions. */
} LJWasmJITModule;

/*
** The embedding host must provide these imports for a full WASM target.
** They intentionally use C ABI-sized values only, so the same header can be
** consumed by WASI hosts and native embedding shims.
*/
LJ_FUNC int lj_wasm_host_ffi_load(const char *name, int global,
				  LJWasmHostHandle *handle);
LJ_FUNC void lj_wasm_host_ffi_unload(LJWasmHostHandle handle);
LJ_FUNC int lj_wasm_host_ffi_symbol(LJWasmHostHandle handle, const char *name,
				    LJWasmHostHandle *symbol);
LJ_FUNC int lj_wasm_host_ffi_call(struct CTState *cts, struct CType *ct,
				  struct CCallState *cc,
				  const LJWasmFFICall *call);
LJ_FUNC int lj_wasm_host_ffi_callback_new(uint32_t slot, uint32_t ctypeid,
					  LJWasmHostHandle *handle);
LJ_FUNC int lj_wasm_host_ffi_callback_slot(LJWasmHostHandle handle,
					   uint32_t *slot);
LJ_FUNC void lj_wasm_host_ffi_callback_free(LJWasmHostHandle handle);

LJ_FUNC int lj_wasm_host_jit_compile(const LJWasmJITModule *module,
				     LJWasmHostHandle *handle);
LJ_FUNC void lj_wasm_host_jit_free(LJWasmHostHandle handle);
LJ_FUNC int lj_wasm_host_jit_enter(LJWasmHostHandle handle, void *lua_state,
				   void *base, void *exit_state,
				   uint32_t exitno);
LJ_FUNC int lj_wasm_host_jit_patch_exit(LJWasmHostHandle from,
					uint32_t exitno,
					LJWasmHostHandle to);

#endif

#endif
