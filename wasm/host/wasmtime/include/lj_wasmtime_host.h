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
#define LJ_WASMTIME_IMPORT_FFI_CALLBACK_NEW "lj_wasm_import_ffi_callback_new"
#define LJ_WASMTIME_IMPORT_FFI_CALLBACK_SLOT "lj_wasm_import_ffi_callback_slot"
#define LJ_WASMTIME_IMPORT_FFI_CALLBACK_FREE "lj_wasm_import_ffi_callback_free"
#define LJ_WASMTIME_IMPORT_JIT_COMPILE "lj_wasm_import_jit_compile"
#define LJ_WASMTIME_IMPORT_JIT_FREE "lj_wasm_import_jit_free"
#define LJ_WASMTIME_IMPORT_JIT_ENTER "lj_wasm_import_jit_enter"
#define LJ_WASMTIME_IMPORT_JIT_PATCH_EXIT "lj_wasm_import_jit_patch_exit"

#define LJ_WASMTIME_HANDLE_MAX 1024u
#define LJ_WASMTIME_GUEST_MAX_CSTR 4096u
#define LJ_WASMTIME_GUEST_MAX_CCALL_SIZE 4096u

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

#define LJ_WASM_FFI_MAX_ARGS 16u
#define LJ_WASM_FFI_CALL_ABI_VERSION 1u

typedef enum LJWasmFFILoc {
  LJ_WASM_FFI_LOC_NONE,
  LJ_WASM_FFI_LOC_GPR,
  LJ_WASM_FFI_LOC_FPR,
  LJ_WASM_FFI_LOC_STACK,
  LJ_WASM_FFI_LOC_RETREF
} LJWasmFFILoc;

#define LJ_WASM_FFI_SIG_F_VARARG 0x01u
#define LJ_WASM_FFI_SIG_F_UNSUPPORTED 0x80u

#define LJ_WASM_FFI_SLOT_F_UNSIGNED 0x01u

typedef struct LJWasmFFISig {
  uint32_t ctypeid;
  uint16_t nargs;
  uint8_t flags;
  uint8_t rettype;
} LJWasmFFISig;

typedef struct LJWasmFFISlot {
  uint8_t type;
  uint8_t loc;
  uint16_t flags;
  uint32_t offset;
  uint32_t size;
} LJWasmFFISlot;

typedef struct LJWasmFFICall {
  LJWasmFFISig sig;
  uint32_t abi_version;
  uint32_t ccall_size;
  uint32_t func_offset;
  uint32_t reserved;
  LJWasmFFISlot ret;
  LJWasmFFISlot args[LJ_WASM_FFI_MAX_ARGS];
} LJWasmFFICall;

#define LJ_WASM_JIT_F_IR_LOWERED 0x00000001u
#define LJ_WASM_JIT_F_IMPORT_ENV_MEMORY 0x00000002u

#define LJ_WASM_JIT_MEMORY_F_64 0x00000001u
#define LJ_WASM_JIT_MEMORY_F_HAS_MAX 0x00000002u

typedef struct LJWasmJITModule {
  const uint8_t *bytes;
  size_t size;
  uint32_t trace;
  uint32_t entry;
  uint32_t exit;
  uint32_t flags;
  uint64_t memory_min;
  uint64_t memory_max;
  uint32_t memory_flags;
  uint32_t reserved;
} LJWasmJITModule;

typedef int (*LJWasmtimeFFILoadFn)(void *ctx, const char *name, int global,
                                   LJWasmHostHandle *handle);
typedef void (*LJWasmtimeFFIUnloadFn)(void *ctx, LJWasmHostHandle handle);
typedef int (*LJWasmtimeFFISymbolFn)(void *ctx, LJWasmHostHandle handle,
                                     const char *name,
                                     LJWasmHostHandle *symbol);
typedef int (*LJWasmtimeFFICallFn)(void *ctx, struct CTState *cts,
                                   struct CType *ct, struct CCallState *cc,
                                   const LJWasmFFICall *call);
typedef int (*LJWasmtimeFFICallbackNewFn)(void *ctx, uint32_t slot,
                                          uint32_t ctypeid,
                                          LJWasmHostHandle *handle);
typedef int (*LJWasmtimeFFICallbackSlotFn)(void *ctx,
                                           LJWasmHostHandle handle,
                                           uint32_t *slot);
typedef void (*LJWasmtimeFFICallbackFreeFn)(void *ctx,
                                            LJWasmHostHandle handle);
typedef int (*LJWasmtimeJITCompileFn)(void *ctx,
                                      const LJWasmJITModule *module,
                                      LJWasmHostHandle *handle);
typedef void (*LJWasmtimeJITFreeFn)(void *ctx, LJWasmHostHandle handle);
typedef int (*LJWasmtimeJITEnterFn)(void *ctx, LJWasmHostHandle handle,
                                    void *lua_state, void *base,
                                    void *exit_state, uint32_t exitno);
typedef int (*LJWasmtimeJITPatchExitFn)(void *ctx, LJWasmHostHandle from,
                                        uint32_t exitno,
                                        LJWasmHostHandle to);

typedef struct LJWasmtimeHostHooks {
  void *ctx;
  LJWasmtimeFFILoadFn ffi_load;
  LJWasmtimeFFIUnloadFn ffi_unload;
  LJWasmtimeFFISymbolFn ffi_symbol;
  LJWasmtimeFFICallFn ffi_call;
  LJWasmtimeFFICallbackNewFn ffi_callback_new;
  LJWasmtimeFFICallbackSlotFn ffi_callback_slot;
  LJWasmtimeFFICallbackFreeFn ffi_callback_free;
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

typedef struct LJWasmtimeGuestMemory {
  uint8_t *data;
  uint64_t size;
} LJWasmtimeGuestMemory;

typedef struct LJWasmtimeHandleTable {
  LJWasmHostHandle slots[LJ_WASMTIME_HANDLE_MAX];
} LJWasmtimeHandleTable;

typedef struct LJWasmtimeGuestContext {
  LJWasmtimeGuestMemory memory;
  LJWasmtimeHandleTable handles;
} LJWasmtimeGuestContext;

extern const LJWasmtimeImportSpec lj_wasmtime_host_imports[];
extern const size_t lj_wasmtime_host_import_count;

void lj_wasmtime_host_set_hooks(const LJWasmtimeHostHooks *hooks);
void lj_wasmtime_host_clear_hooks(void);
const char *lj_wasmtime_host_status_name(int status);
int lj_wasmtime_host_validate_jit_module(const LJWasmJITModule *module);
int lj_wasmtime_guest_ptr(const LJWasmtimeGuestMemory *memory,
                          uint64_t guest_ptr, size_t size, void **host_ptr);
int lj_wasmtime_guest_read(const LJWasmtimeGuestMemory *memory,
                           uint64_t guest_ptr, void *dst, size_t size);
int lj_wasmtime_guest_write(const LJWasmtimeGuestMemory *memory,
                            uint64_t guest_ptr, const void *src, size_t size);
int lj_wasmtime_guest_read_cstr(const LJWasmtimeGuestMemory *memory,
                                uint64_t guest_ptr, char *dst,
                                size_t dst_size);
void lj_wasmtime_guest_context_init(LJWasmtimeGuestContext *ctx,
                                    uint8_t *memory, uint64_t memory_size);
int lj_wasmtime_guest_handle_alloc(LJWasmtimeHandleTable *handles,
                                   LJWasmHostHandle host_handle,
                                   uint64_t *guest_handle);
int lj_wasmtime_guest_handle_get(const LJWasmtimeHandleTable *handles,
                                 uint64_t guest_handle,
                                 LJWasmHostHandle *host_handle);
void lj_wasmtime_guest_handle_release(LJWasmtimeHandleTable *handles,
                                      uint64_t guest_handle);
int lj_wasmtime_guest_ffi_load(LJWasmtimeGuestContext *ctx,
                               uint64_t name_ptr, int global,
                               uint64_t handle_out_ptr);
void lj_wasmtime_guest_ffi_unload(LJWasmtimeGuestContext *ctx,
                                  uint64_t guest_handle);
int lj_wasmtime_guest_ffi_symbol(LJWasmtimeGuestContext *ctx,
                                 uint64_t guest_handle, uint64_t name_ptr,
                                 uint64_t symbol_out_ptr);
int lj_wasmtime_guest_ffi_call(LJWasmtimeGuestContext *ctx, uint64_t cts_ptr,
                               uint64_t ct_ptr, uint64_t cc_ptr,
                               uint64_t call_ptr);

int lj_wasm_import_ffi_load(const char *name, int global,
                            LJWasmHostHandle *handle);
void lj_wasm_import_ffi_unload(LJWasmHostHandle handle);
int lj_wasm_import_ffi_symbol(LJWasmHostHandle handle, const char *name,
                              LJWasmHostHandle *symbol);
int lj_wasm_import_ffi_call(struct CTState *cts, struct CType *ct,
                            struct CCallState *cc,
                            const LJWasmFFICall *call);
int lj_wasm_import_ffi_callback_new(uint32_t slot, uint32_t ctypeid,
                                    LJWasmHostHandle *handle);
int lj_wasm_import_ffi_callback_slot(LJWasmHostHandle handle, uint32_t *slot);
void lj_wasm_import_ffi_callback_free(LJWasmHostHandle handle);

int lj_wasm_import_jit_compile(const LJWasmJITModule *module,
                               LJWasmHostHandle *handle);
void lj_wasm_import_jit_free(LJWasmHostHandle handle);
int lj_wasm_import_jit_enter(LJWasmHostHandle handle, void *lua_state,
                             void *base, void *exit_state, uint32_t exitno);
int lj_wasm_import_jit_patch_exit(LJWasmHostHandle from, uint32_t exitno,
                                  LJWasmHostHandle to);

#ifdef __cplusplus
}
#endif

#endif
