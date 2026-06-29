#include "lj_wasmtime_host.h"

#include <string.h>

const LJWasmtimeImportSpec lj_wasmtime_host_imports[] = {
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_LOAD,
     "(name: guest_ptr, global: i32, handle_out: guest_ptr) -> i32",
     "Load a host FFI library or global namespace."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_UNLOAD,
     "(handle: host_handle) -> void", "Release a loaded FFI library handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_SYMBOL,
     "(handle: host_handle, name: guest_ptr, symbol_out: guest_ptr) -> i32",
     "Resolve a symbol from a loaded FFI handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_CALL,
     "(cts: guest_ptr, ct: guest_ptr, cc: guest_ptr, call: guest_ptr) -> i32",
     "Marshal and perform an FFI call described by LuaJIT C state and a scalar call descriptor."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_CALLBACK_NEW,
     "(slot: u32, ctypeid: u32, handle_out: guest_ptr) -> i32",
     "Create a callable host/table callback handle for a LuaJIT callback slot."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_CALLBACK_SLOT,
     "(handle: host_handle, slot_out: guest_ptr) -> i32",
     "Resolve a callback handle back to its LuaJIT callback slot."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_FFI_CALLBACK_FREE,
     "(handle: host_handle) -> void", "Release a callback handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_COMPILE,
     "(module: guest_ptr, handle_out: guest_ptr) -> i32",
     "Compile a guest-provided trace module and return a trace handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_FREE,
     "(handle: host_handle) -> void", "Release a compiled trace handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_ENTER,
     "(handle: host_handle, lua_state: guest_ptr, base: guest_ptr, exit_state: guest_ptr, exitno: u32) -> i32",
     "Enter a compiled trace export."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_PATCH_EXIT,
     "(from: host_handle, exitno: u32, to: host_handle) -> i32",
     "Patch one compiled trace exit to another trace handle."},
};

const size_t lj_wasmtime_host_import_count =
    sizeof(lj_wasmtime_host_imports) / sizeof(lj_wasmtime_host_imports[0]);

static LJWasmtimeHostHooks lj_wasmtime_hooks;

void lj_wasmtime_host_set_hooks(const LJWasmtimeHostHooks *hooks) {
  if (hooks == NULL) {
    lj_wasmtime_host_clear_hooks();
    return;
  }

  lj_wasmtime_hooks = *hooks;
}

void lj_wasmtime_host_clear_hooks(void) {
  memset(&lj_wasmtime_hooks, 0, sizeof(lj_wasmtime_hooks));
}

const char *lj_wasmtime_host_status_name(int status) {
  switch (status) {
  case LJ_WASM_HOST_OK:
    return "ok";
  case LJ_WASM_HOST_ERR:
    return "error";
  case LJ_WASM_HOST_NYI:
    return "not-yet-implemented";
  default:
    return "unknown";
  }
}

int lj_wasmtime_host_validate_jit_module(const LJWasmJITModule *module) {
  const uint32_t known_flags =
      LJ_WASM_JIT_F_IR_LOWERED | LJ_WASM_JIT_F_IMPORT_ENV_MEMORY;
  const uint32_t known_memory_flags =
      LJ_WASM_JIT_MEMORY_F_64 | LJ_WASM_JIT_MEMORY_F_HAS_MAX;

  if (module == NULL || module->bytes == NULL || module->size == 0 ||
      module->reserved != 0 || (module->flags & ~known_flags) != 0 ||
      (module->memory_flags & ~known_memory_flags) != 0) {
    return LJ_WASM_HOST_ERR;
  }

  if ((module->flags & LJ_WASM_JIT_F_IMPORT_ENV_MEMORY) == 0) {
    if (module->memory_min != 0 || module->memory_max != 0 ||
        module->memory_flags != 0) {
      return LJ_WASM_HOST_ERR;
    }
    return LJ_WASM_HOST_OK;
  }

  if ((module->memory_flags & LJ_WASM_JIT_MEMORY_F_64) == 0 ||
      module->memory_min == 0) {
    return LJ_WASM_HOST_ERR;
  }

  if ((module->memory_flags & LJ_WASM_JIT_MEMORY_F_HAS_MAX) != 0) {
    if (module->memory_max < module->memory_min) {
      return LJ_WASM_HOST_ERR;
    }
  } else if (module->memory_max != 0) {
    return LJ_WASM_HOST_ERR;
  }

  return LJ_WASM_HOST_OK;
}

int lj_wasm_import_ffi_load(const char *name, int global,
                            LJWasmHostHandle *handle) {
  if (handle != NULL) {
    *handle = NULL;
  }
  if (name == NULL || handle == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_load != NULL) {
    return lj_wasmtime_hooks.ffi_load(lj_wasmtime_hooks.ctx, name, global,
                                      handle);
  }
  return LJ_WASM_HOST_NYI;
}

void lj_wasm_import_ffi_unload(LJWasmHostHandle handle) {
  if (lj_wasmtime_hooks.ffi_unload != NULL) {
    lj_wasmtime_hooks.ffi_unload(lj_wasmtime_hooks.ctx, handle);
  }
}

int lj_wasm_import_ffi_symbol(LJWasmHostHandle handle, const char *name,
                              LJWasmHostHandle *symbol) {
  if (symbol != NULL) {
    *symbol = NULL;
  }
  if (handle == NULL || name == NULL || symbol == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_symbol != NULL) {
    return lj_wasmtime_hooks.ffi_symbol(lj_wasmtime_hooks.ctx, handle, name,
                                        symbol);
  }
  return LJ_WASM_HOST_NYI;
}

int lj_wasm_import_ffi_call(struct CTState *cts, struct CType *ct,
                            struct CCallState *cc,
                            const LJWasmFFICall *call) {
  if (cts == NULL || ct == NULL || cc == NULL || call == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_call != NULL) {
    return lj_wasmtime_hooks.ffi_call(lj_wasmtime_hooks.ctx, cts, ct, cc,
                                      call);
  }
  return LJ_WASM_HOST_NYI;
}

int lj_wasm_import_ffi_callback_new(uint32_t slot, uint32_t ctypeid,
                                    LJWasmHostHandle *handle) {
  if (handle != NULL) {
    *handle = NULL;
  }
  if (handle == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_callback_new != NULL) {
    return lj_wasmtime_hooks.ffi_callback_new(lj_wasmtime_hooks.ctx, slot,
                                              ctypeid, handle);
  }
  return LJ_WASM_HOST_NYI;
}

int lj_wasm_import_ffi_callback_slot(LJWasmHostHandle handle, uint32_t *slot) {
  if (slot != NULL) {
    *slot = UINT32_MAX;
  }
  if (handle == NULL || slot == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_callback_slot != NULL) {
    return lj_wasmtime_hooks.ffi_callback_slot(lj_wasmtime_hooks.ctx, handle,
                                               slot);
  }
  return LJ_WASM_HOST_NYI;
}

void lj_wasm_import_ffi_callback_free(LJWasmHostHandle handle) {
  if (lj_wasmtime_hooks.ffi_callback_free != NULL) {
    lj_wasmtime_hooks.ffi_callback_free(lj_wasmtime_hooks.ctx, handle);
  }
}

int lj_wasm_import_jit_compile(const LJWasmJITModule *module,
                               LJWasmHostHandle *handle) {
  if (handle != NULL) {
    *handle = NULL;
  }
  if (handle == NULL ||
      lj_wasmtime_host_validate_jit_module(module) != LJ_WASM_HOST_OK) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.jit_compile != NULL) {
    return lj_wasmtime_hooks.jit_compile(lj_wasmtime_hooks.ctx, module,
                                         handle);
  }
  return LJ_WASM_HOST_NYI;
}

void lj_wasm_import_jit_free(LJWasmHostHandle handle) {
  if (lj_wasmtime_hooks.jit_free != NULL) {
    lj_wasmtime_hooks.jit_free(lj_wasmtime_hooks.ctx, handle);
  }
}

int lj_wasm_import_jit_enter(LJWasmHostHandle handle, void *lua_state,
                             void *base, void *exit_state, uint32_t exitno) {
  if (handle == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.jit_enter != NULL) {
    return lj_wasmtime_hooks.jit_enter(lj_wasmtime_hooks.ctx, handle,
                                       lua_state, base, exit_state, exitno);
  }
  return LJ_WASM_HOST_NYI;
}

int lj_wasm_import_jit_patch_exit(LJWasmHostHandle from, uint32_t exitno,
                                  LJWasmHostHandle to) {
  if (from == NULL || to == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.jit_patch_exit != NULL) {
    return lj_wasmtime_hooks.jit_patch_exit(lj_wasmtime_hooks.ctx, from,
                                            exitno, to);
  }
  return LJ_WASM_HOST_NYI;
}
