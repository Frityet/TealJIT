#ifndef LJ_WASMTIME_HOST_H
#define LJ_WASMTIME_HOST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LJ_WASMTIME_IMPORT_MODULE "env"

#define LJ_WASMTIME_IMPORT_FFI_LOAD "lj_wasm_import_ffi_load"
#define LJ_WASMTIME_IMPORT_FFI_UNLOAD "lj_wasm_import_ffi_unload"
#define LJ_WASMTIME_IMPORT_FFI_SYMBOL "lj_wasm_import_ffi_symbol"
#define LJ_WASMTIME_IMPORT_FFI_CALL "lj_wasm_import_ffi_call"
#define LJ_WASMTIME_IMPORT_JIT_COMPILE "lj_wasm_import_jit_compile"
#define LJ_WASMTIME_IMPORT_JIT_FREE "lj_wasm_import_jit_free"
#define LJ_WASMTIME_IMPORT_JIT_ENTER "lj_wasm_import_jit_enter"
#define LJ_WASMTIME_IMPORT_JIT_PATCH_EXIT "lj_wasm_import_jit_patch_exit"

struct CCallState;
struct CTState;
struct CType;

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

typedef struct LJWasmFFISig {
  uint32_t ctypeid;
  uint16_t nargs;
  uint8_t flags;
  uint8_t rettype;
} LJWasmFFISig;

typedef struct LJWasmJITModule {
  const uint8_t *bytes;
  size_t size;
  uint32_t trace;
  uint32_t entry;
  uint32_t exit;
  uint32_t flags;
} LJWasmJITModule;

typedef int (*LJWasmtimeFFILoadFn)(void *ctx, const char *name, int global,
                                   LJWasmHostHandle *handle);
typedef void (*LJWasmtimeFFIUnloadFn)(void *ctx, LJWasmHostHandle handle);
typedef int (*LJWasmtimeFFISymbolFn)(void *ctx, LJWasmHostHandle handle,
                                     const char *name,
                                     LJWasmHostHandle *symbol);
typedef int (*LJWasmtimeFFICallFn)(void *ctx, struct CTState *cts,
                                   struct CType *ct, struct CCallState *cc);
typedef int (*LJWasmtimeJITCompileFn)(void *ctx,
                                      const LJWasmJITModule *module,
                                      LJWasmHostHandle *handle);
typedef void (*LJWasmtimeJITFreeFn)(void *ctx, LJWasmHostHandle handle);
typedef int (*LJWasmtimeJITEnterFn)(void *ctx, LJWasmHostHandle handle,
                                    void *lua_state, void *base,
                                    uint32_t exitno);
typedef int (*LJWasmtimeJITPatchExitFn)(void *ctx, LJWasmHostHandle from,
                                        uint32_t exitno,
                                        LJWasmHostHandle to);

typedef struct LJWasmtimeHostHooks {
  void *ctx;
  LJWasmtimeFFILoadFn ffi_load;
  LJWasmtimeFFIUnloadFn ffi_unload;
  LJWasmtimeFFISymbolFn ffi_symbol;
  LJWasmtimeFFICallFn ffi_call;
  LJWasmtimeJITCompileFn jit_compile;
  LJWasmtimeJITFreeFn jit_free;
  LJWasmtimeJITEnterFn jit_enter;
  LJWasmtimeJITPatchExitFn jit_patch_exit;
} LJWasmtimeHostHooks;

typedef struct LJWasmtimeImportSpec {
  const char *module;
  const char *name;
  const char *signature;
  const char *summary;
} LJWasmtimeImportSpec;

extern const LJWasmtimeImportSpec lj_wasmtime_host_imports[];
extern const size_t lj_wasmtime_host_import_count;

void lj_wasmtime_host_set_hooks(const LJWasmtimeHostHooks *hooks);
void lj_wasmtime_host_clear_hooks(void);
const char *lj_wasmtime_host_status_name(int status);

int lj_wasm_import_ffi_load(const char *name, int global,
                            LJWasmHostHandle *handle);
void lj_wasm_import_ffi_unload(LJWasmHostHandle handle);
int lj_wasm_import_ffi_symbol(LJWasmHostHandle handle, const char *name,
                              LJWasmHostHandle *symbol);
int lj_wasm_import_ffi_call(struct CTState *cts, struct CType *ct,
                            struct CCallState *cc);

int lj_wasm_import_jit_compile(const LJWasmJITModule *module,
                               LJWasmHostHandle *handle);
void lj_wasm_import_jit_free(LJWasmHostHandle handle);
int lj_wasm_import_jit_enter(LJWasmHostHandle handle, void *lua_state,
                             void *base, uint32_t exitno);
int lj_wasm_import_jit_patch_exit(LJWasmHostHandle from, uint32_t exitno,
                                  LJWasmHostHandle to);

#ifdef __cplusplus
}
#endif

#endif
