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
     "(cts: guest_ptr, ct: guest_ptr, cc: guest_ptr) -> i32",
     "Marshal and perform an FFI call described by LuaJIT C state."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_COMPILE,
     "(module: guest_ptr, handle_out: guest_ptr) -> i32",
     "Compile a guest-provided trace module and return a trace handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_FREE,
     "(handle: host_handle) -> void", "Release a compiled trace handle."},
    {LJ_WASMTIME_IMPORT_MODULE, LJ_WASMTIME_IMPORT_JIT_ENTER,
     "(handle: host_handle, lua_state: guest_ptr, base: guest_ptr, exitno: u32) -> i32",
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
                            struct CCallState *cc) {
  if (cts == NULL || ct == NULL || cc == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_call != NULL) {
    return lj_wasmtime_hooks.ffi_call(lj_wasmtime_hooks.ctx, cts, ct, cc);
  }
  return LJ_WASM_HOST_NYI;
}

int lj_wasm_import_jit_compile(const LJWasmJITModule *module,
                               LJWasmHostHandle *handle) {
  if (handle != NULL) {
    *handle = NULL;
  }
  if (module == NULL || module->bytes == NULL || module->size == 0 ||
      handle == NULL) {
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
                             void *base, uint32_t exitno) {
  if (handle == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.jit_enter != NULL) {
    return lj_wasmtime_hooks.jit_enter(lj_wasmtime_hooks.ctx, handle,
                                       lua_state, base, exitno);
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
