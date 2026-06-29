#include "lj_wasmtime_host.h"

#include <stdlib.h>
#include <string.h>

#if LJ_WASMTIME_ENABLE_LIBFFI
#include <dlfcn.h>
#include <ffi.h>
#endif

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

#if LJ_WASMTIME_ENABLE_LIBFFI
#define LJ_WASMTIME_FFI_HANDLE_MAGIC 0x4c4a4649u
#define LJ_WASMTIME_FFI_KIND_LIB 1u
#define LJ_WASMTIME_FFI_KIND_SYMBOL 2u

typedef struct LJWasmtimeFFIHandle {
  uint32_t magic;
  uint32_t kind;
  void *value;
  struct LJWasmtimeFFIHandle *owner;
} LJWasmtimeFFIHandle;

typedef union LJWasmtimeFFIValue {
  int8_t s8;
  uint8_t u8;
  int16_t s16;
  uint16_t u16;
  int32_t s32;
  uint32_t u32;
  int64_t s64;
  uint64_t u64;
  float f32;
  double f64;
} LJWasmtimeFFIValue;

static LJWasmtimeFFIHandle *lj_wasmtime_ffi_handle(LJWasmHostHandle handle,
                                                   uint32_t kind) {
  LJWasmtimeFFIHandle *h = (LJWasmtimeFFIHandle *)handle;
  if (h == NULL || h->magic != LJ_WASMTIME_FFI_HANDLE_MAGIC ||
      h->kind != kind || h->value == NULL) {
    return NULL;
  }
  return h;
}

static int lj_wasmtime_ffi_new_handle(uint32_t kind, void *value,
                                      LJWasmtimeFFIHandle *owner,
                                      LJWasmHostHandle *out) {
  LJWasmtimeFFIHandle *h;
  if (value == NULL || out == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  h = (LJWasmtimeFFIHandle *)malloc(sizeof(*h));
  if (h == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  h->magic = LJ_WASMTIME_FFI_HANDLE_MAGIC;
  h->kind = kind;
  h->value = value;
  h->owner = owner;
  *out = (LJWasmHostHandle)h;
  return LJ_WASM_HOST_OK;
}

static void lj_wasmtime_ffi_invalidate_handle(LJWasmtimeFFIHandle *h) {
  h->magic = 0;
  h->kind = 0;
  h->value = NULL;
  h->owner = NULL;
}

static ffi_type *lj_wasmtime_ffi_int_type(uint32_t size, uint16_t flags) {
  const int is_unsigned = (flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) != 0;
  switch (size) {
  case 1:
    return is_unsigned ? &ffi_type_uint8 : &ffi_type_sint8;
  case 2:
    return is_unsigned ? &ffi_type_uint16 : &ffi_type_sint16;
  case 4:
    return is_unsigned ? &ffi_type_uint32 : &ffi_type_sint32;
  case 8:
    return is_unsigned ? &ffi_type_uint64 : &ffi_type_sint64;
  default:
    return NULL;
  }
}

static ffi_type *lj_wasmtime_ffi_slot_type(const LJWasmFFISlot *slot) {
  switch ((LJWasmScalarType)slot->type) {
  case LJ_WASM_SCALAR_VOID:
    return &ffi_type_void;
  case LJ_WASM_SCALAR_I32:
  case LJ_WASM_SCALAR_I64:
    return lj_wasmtime_ffi_int_type(slot->size, slot->flags);
  case LJ_WASM_SCALAR_F32:
    return slot->size == sizeof(float) ? &ffi_type_float : NULL;
  case LJ_WASM_SCALAR_F64:
    return slot->size == sizeof(double) ? &ffi_type_double : NULL;
  case LJ_WASM_SCALAR_PTR:
  case LJ_WASM_SCALAR_AGG:
  default:
    return NULL;
  }
}

static int lj_wasmtime_ffi_slot_has_value_loc(const LJWasmFFISlot *slot) {
  return slot->loc == LJ_WASM_FFI_LOC_GPR ||
         slot->loc == LJ_WASM_FFI_LOC_FPR ||
         slot->loc == LJ_WASM_FFI_LOC_STACK;
}

static void *lj_wasmtime_ffi_cc_ptr(struct CCallState *cc, uint32_t offset) {
  return (void *)((uint8_t *)cc + offset);
}

static int lj_wasmtime_ffi_range_ok(const LJWasmFFICall *call,
                                    uint32_t offset, uint32_t size) {
  return size <= call->ccall_size && offset <= call->ccall_size - size;
}

static int lj_wasmtime_ffi_slot_range_ok(const LJWasmFFICall *call,
                                         const LJWasmFFISlot *slot) {
  return slot->size > 0 && lj_wasmtime_ffi_range_ok(call, slot->offset,
                                                    slot->size);
}

static int lj_wasmtime_ffi_sig_status(const LJWasmFFICall *call) {
  const uint8_t known_flags =
      LJ_WASM_FFI_SIG_F_VARARG | LJ_WASM_FFI_SIG_F_UNSUPPORTED;
  if (call->abi_version != LJ_WASM_FFI_CALL_ABI_VERSION ||
      call->ccall_size == 0 || call->reserved != 0 ||
      call->sig.rettype != call->ret.type ||
      call->sig.nargs > LJ_WASM_FFI_MAX_ARGS ||
      (call->sig.flags & (uint8_t)~known_flags) != 0) {
    return LJ_WASM_HOST_ERR;
  }
  if ((call->sig.flags &
       (LJ_WASM_FFI_SIG_F_UNSUPPORTED | LJ_WASM_FFI_SIG_F_VARARG)) != 0) {
    return LJ_WASM_HOST_NYI;
  }
  if (!lj_wasmtime_ffi_range_ok(call, call->func_offset,
                                sizeof(LJWasmHostHandle))) {
    return LJ_WASM_HOST_ERR;
  }
  return LJ_WASM_HOST_OK;
}

static int lj_wasmtime_ffi_slot_status(const LJWasmFFICall *call,
                                       const LJWasmFFISlot *slot,
                                       int is_ret) {
  if (slot->flags & (uint16_t)~LJ_WASM_FFI_SLOT_F_UNSIGNED) {
    return LJ_WASM_HOST_ERR;
  }
  if (slot->type > LJ_WASM_SCALAR_AGG) {
    return LJ_WASM_HOST_ERR;
  }
  if (slot->type == LJ_WASM_SCALAR_AGG || slot->type == LJ_WASM_SCALAR_PTR ||
      slot->loc == LJ_WASM_FFI_LOC_RETREF) {
    return LJ_WASM_HOST_NYI;
  }
  if (slot->type == LJ_WASM_SCALAR_VOID) {
    return is_ret && slot->loc == LJ_WASM_FFI_LOC_NONE ?
           LJ_WASM_HOST_OK : LJ_WASM_HOST_ERR;
  }
  if (!lj_wasmtime_ffi_slot_has_value_loc(slot) ||
      !lj_wasmtime_ffi_slot_range_ok(call, slot)) {
    return LJ_WASM_HOST_ERR;
  }
  return lj_wasmtime_ffi_slot_type(slot) != NULL ? LJ_WASM_HOST_OK :
         LJ_WASM_HOST_NYI;
}

static int lj_wasmtime_ffi_load_value(struct CCallState *cc,
                                      const LJWasmFFISlot *slot,
                                      LJWasmtimeFFIValue *value) {
  void *src = lj_wasmtime_ffi_cc_ptr(cc, slot->offset);
  switch ((LJWasmScalarType)slot->type) {
  case LJ_WASM_SCALAR_I32:
  case LJ_WASM_SCALAR_I64:
    switch (slot->size) {
    case 1:
      if (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) {
        memcpy(&value->u8, src, sizeof(value->u8));
      } else {
        memcpy(&value->s8, src, sizeof(value->s8));
      }
      return LJ_WASM_HOST_OK;
    case 2:
      if (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) {
        memcpy(&value->u16, src, sizeof(value->u16));
      } else {
        memcpy(&value->s16, src, sizeof(value->s16));
      }
      return LJ_WASM_HOST_OK;
    case 4:
      if (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) {
        memcpy(&value->u32, src, sizeof(value->u32));
      } else {
        memcpy(&value->s32, src, sizeof(value->s32));
      }
      return LJ_WASM_HOST_OK;
    case 8:
      if (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) {
        memcpy(&value->u64, src, sizeof(value->u64));
      } else {
        memcpy(&value->s64, src, sizeof(value->s64));
      }
      return LJ_WASM_HOST_OK;
    default:
      return LJ_WASM_HOST_NYI;
    }
  case LJ_WASM_SCALAR_F32:
    memcpy(&value->f32, src, sizeof(value->f32));
    return LJ_WASM_HOST_OK;
  case LJ_WASM_SCALAR_F64:
    memcpy(&value->f64, src, sizeof(value->f64));
    return LJ_WASM_HOST_OK;
  default:
    return LJ_WASM_HOST_NYI;
  }
}

static int lj_wasmtime_ffi_store_value(struct CCallState *cc,
                                       const LJWasmFFISlot *slot,
                                       const LJWasmtimeFFIValue *value) {
  void *dst = lj_wasmtime_ffi_cc_ptr(cc, slot->offset);
  switch ((LJWasmScalarType)slot->type) {
  case LJ_WASM_SCALAR_I32:
  case LJ_WASM_SCALAR_I64:
    switch (slot->size) {
    case 1:
      memcpy(dst, (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) ?
                  (const void *)&value->u8 : (const void *)&value->s8,
             sizeof(value->u8));
      return LJ_WASM_HOST_OK;
    case 2:
      memcpy(dst, (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) ?
                  (const void *)&value->u16 : (const void *)&value->s16,
             sizeof(value->u16));
      return LJ_WASM_HOST_OK;
    case 4:
      memcpy(dst, (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) ?
                  (const void *)&value->u32 : (const void *)&value->s32,
             sizeof(value->u32));
      return LJ_WASM_HOST_OK;
    case 8:
      memcpy(dst, (slot->flags & LJ_WASM_FFI_SLOT_F_UNSIGNED) ?
                  (const void *)&value->u64 : (const void *)&value->s64,
             sizeof(value->u64));
      return LJ_WASM_HOST_OK;
    default:
      return LJ_WASM_HOST_NYI;
    }
  case LJ_WASM_SCALAR_F32:
    memcpy(dst, &value->f32, sizeof(value->f32));
    return LJ_WASM_HOST_OK;
  case LJ_WASM_SCALAR_F64:
    memcpy(dst, &value->f64, sizeof(value->f64));
    return LJ_WASM_HOST_OK;
  default:
    return LJ_WASM_HOST_NYI;
  }
}

static int lj_wasmtime_ffi_read_func(struct CCallState *cc,
                                     const LJWasmFFICall *call,
                                     void (**fn)(void)) {
  LJWasmHostHandle handle;
  LJWasmtimeFFIHandle *symbol;
  if (sizeof(handle) != sizeof(*fn) || sizeof(handle) != sizeof(symbol)) {
    return LJ_WASM_HOST_ERR;
  }
  memcpy(&handle, lj_wasmtime_ffi_cc_ptr(cc, call->func_offset),
         sizeof(handle));
  symbol = lj_wasmtime_ffi_handle(handle, LJ_WASMTIME_FFI_KIND_SYMBOL);
  if (symbol == NULL ||
      (symbol->owner != NULL &&
       symbol->owner->magic != LJ_WASMTIME_FFI_HANDLE_MAGIC)) {
    return LJ_WASM_HOST_ERR;
  }
  memcpy(fn, &symbol->value, sizeof(*fn));
  return LJ_WASM_HOST_OK;
}

static int lj_wasmtime_ffi_default_call(struct CCallState *cc,
                                        const LJWasmFFICall *call) {
  ffi_type *arg_types[LJ_WASM_FFI_MAX_ARGS];
  void *arg_values[LJ_WASM_FFI_MAX_ARGS];
  LJWasmtimeFFIValue arg_storage[LJ_WASM_FFI_MAX_ARGS];
  LJWasmtimeFFIValue ret_storage;
  ffi_type *ret_type;
  ffi_cif cif;
  void *ret_value = NULL;
  void (*fn)(void);
  int status;
  uint16_t i;

  status = lj_wasmtime_ffi_sig_status(call);
  if (status != LJ_WASM_HOST_OK)
    return status;

  ret_type = lj_wasmtime_ffi_slot_type(&call->ret);
  status = lj_wasmtime_ffi_slot_status(call, &call->ret, 1);
  if (status != LJ_WASM_HOST_OK)
    return status;
  if (call->ret.type != LJ_WASM_SCALAR_VOID) {
    ret_value = &ret_storage;
  }

  for (i = 0; i < call->sig.nargs; i++) {
    const LJWasmFFISlot *slot = &call->args[i];
    status = lj_wasmtime_ffi_slot_status(call, slot, 0);
    if (status != LJ_WASM_HOST_OK)
      return status;
    arg_types[i] = lj_wasmtime_ffi_slot_type(slot);
    status = lj_wasmtime_ffi_load_value(cc, slot, &arg_storage[i]);
    if (status != LJ_WASM_HOST_OK)
      return status;
    arg_values[i] = &arg_storage[i];
  }

  if (lj_wasmtime_ffi_read_func(cc, call, &fn) != LJ_WASM_HOST_OK) {
    return LJ_WASM_HOST_ERR;
  }
  if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, call->sig.nargs, ret_type,
                   arg_types) != FFI_OK) {
    return LJ_WASM_HOST_ERR;
  }
  ffi_call(&cif, fn, ret_value, arg_values);
  if (call->ret.type != LJ_WASM_SCALAR_VOID)
    return lj_wasmtime_ffi_store_value(cc, &call->ret, &ret_storage);
  return LJ_WASM_HOST_OK;
}
#endif

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

int lj_wasmtime_guest_ptr(const LJWasmtimeGuestMemory *memory,
                          uint64_t guest_ptr, size_t size, void **host_ptr) {
  if (host_ptr != NULL) {
    *host_ptr = NULL;
  }
  if (memory == NULL || memory->data == NULL || host_ptr == NULL ||
      size > memory->size || guest_ptr > memory->size - size) {
    return LJ_WASM_HOST_ERR;
  }
  *host_ptr = (void *)(memory->data + guest_ptr);
  return LJ_WASM_HOST_OK;
}

int lj_wasmtime_guest_read(const LJWasmtimeGuestMemory *memory,
                           uint64_t guest_ptr, void *dst, size_t size) {
  void *src;
  if (dst == NULL && size != 0) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_guest_ptr(memory, guest_ptr, size, &src) !=
      LJ_WASM_HOST_OK) {
    return LJ_WASM_HOST_ERR;
  }
  if (size != 0) {
    memcpy(dst, src, size);
  }
  return LJ_WASM_HOST_OK;
}

int lj_wasmtime_guest_write(const LJWasmtimeGuestMemory *memory,
                            uint64_t guest_ptr, const void *src,
                            size_t size) {
  void *dst;
  if (src == NULL && size != 0) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_guest_ptr(memory, guest_ptr, size, &dst) !=
      LJ_WASM_HOST_OK) {
    return LJ_WASM_HOST_ERR;
  }
  if (size != 0) {
    memcpy(dst, src, size);
  }
  return LJ_WASM_HOST_OK;
}

int lj_wasmtime_guest_read_cstr(const LJWasmtimeGuestMemory *memory,
                                uint64_t guest_ptr, char *dst,
                                size_t dst_size) {
  uint64_t i;
  if (dst == NULL || dst_size == 0 || memory == NULL || memory->data == NULL ||
      guest_ptr >= memory->size) {
    return LJ_WASM_HOST_ERR;
  }
  for (i = guest_ptr; i < memory->size; i++) {
    uint64_t len = i - guest_ptr;
    if (len + 1 >= dst_size) {
      return LJ_WASM_HOST_ERR;
    }
    dst[len] = (char)memory->data[i];
    if (memory->data[i] == 0) {
      return LJ_WASM_HOST_OK;
    }
  }
  dst[0] = 0;
  return LJ_WASM_HOST_ERR;
}

void lj_wasmtime_guest_context_init(LJWasmtimeGuestContext *ctx,
                                    uint8_t *memory, uint64_t memory_size) {
  if (ctx == NULL) {
    return;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->memory.data = memory;
  ctx->memory.size = memory_size;
}

int lj_wasmtime_guest_handle_alloc(LJWasmtimeHandleTable *handles,
                                   LJWasmHostHandle host_handle,
                                   uint64_t *guest_handle) {
  size_t i;
  if (guest_handle != NULL) {
    *guest_handle = 0;
  }
  if (handles == NULL || host_handle == NULL || guest_handle == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  for (i = 1; i < LJ_WASMTIME_HANDLE_MAX; i++) {
    if (handles->slots[i] == NULL) {
      handles->slots[i] = host_handle;
      *guest_handle = (uint64_t)i;
      return LJ_WASM_HOST_OK;
    }
  }
  return LJ_WASM_HOST_ERR;
}

int lj_wasmtime_guest_handle_get(const LJWasmtimeHandleTable *handles,
                                 uint64_t guest_handle,
                                 LJWasmHostHandle *host_handle) {
  if (host_handle != NULL) {
    *host_handle = NULL;
  }
  if (handles == NULL || host_handle == NULL ||
      guest_handle >= LJ_WASMTIME_HANDLE_MAX ||
      handles->slots[guest_handle] == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  *host_handle = handles->slots[guest_handle];
  return LJ_WASM_HOST_OK;
}

void lj_wasmtime_guest_handle_release(LJWasmtimeHandleTable *handles,
                                      uint64_t guest_handle) {
  if (handles != NULL && guest_handle > 0 &&
      guest_handle < LJ_WASMTIME_HANDLE_MAX) {
    handles->slots[guest_handle] = NULL;
  }
}

static int lj_wasmtime_guest_write_handle(LJWasmtimeGuestContext *ctx,
                                          uint64_t out_ptr,
                                          uint64_t guest_handle) {
  return lj_wasmtime_guest_write(&ctx->memory, out_ptr, &guest_handle,
                                 sizeof(guest_handle));
}

int lj_wasmtime_guest_ffi_load(LJWasmtimeGuestContext *ctx,
                               uint64_t name_ptr, int global,
                               uint64_t handle_out_ptr) {
  char name[LJ_WASMTIME_GUEST_MAX_CSTR];
  LJWasmHostHandle host_handle = NULL;
  uint64_t guest_handle = 0;
  int status;
  if (ctx == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  status = lj_wasmtime_guest_read_cstr(&ctx->memory, name_ptr, name,
                                       sizeof(name));
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  status = lj_wasm_import_ffi_load(name, global, &host_handle);
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  status = lj_wasmtime_guest_handle_alloc(&ctx->handles, host_handle,
                                          &guest_handle);
  if (status != LJ_WASM_HOST_OK) {
    lj_wasm_import_ffi_unload(host_handle);
    return status;
  }
  status = lj_wasmtime_guest_write_handle(ctx, handle_out_ptr, guest_handle);
  if (status != LJ_WASM_HOST_OK) {
    lj_wasm_import_ffi_unload(host_handle);
    lj_wasmtime_guest_handle_release(&ctx->handles, guest_handle);
  }
  return status;
}

void lj_wasmtime_guest_ffi_unload(LJWasmtimeGuestContext *ctx,
                                  uint64_t guest_handle) {
  LJWasmHostHandle host_handle = NULL;
  if (ctx == NULL || guest_handle == 0) {
    return;
  }
  if (lj_wasmtime_guest_handle_get(&ctx->handles, guest_handle,
                                   &host_handle) == LJ_WASM_HOST_OK) {
    lj_wasm_import_ffi_unload(host_handle);
    lj_wasmtime_guest_handle_release(&ctx->handles, guest_handle);
  }
}

int lj_wasmtime_guest_ffi_symbol(LJWasmtimeGuestContext *ctx,
                                 uint64_t guest_handle, uint64_t name_ptr,
                                 uint64_t symbol_out_ptr) {
  char name[LJ_WASMTIME_GUEST_MAX_CSTR];
  LJWasmHostHandle host_handle = NULL;
  LJWasmHostHandle host_symbol = NULL;
  uint64_t guest_symbol = 0;
  int status;
  if (ctx == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (guest_handle != 0) {
    status = lj_wasmtime_guest_handle_get(&ctx->handles, guest_handle,
                                          &host_handle);
    if (status != LJ_WASM_HOST_OK) {
      return status;
    }
  }
  status = lj_wasmtime_guest_read_cstr(&ctx->memory, name_ptr, name,
                                       sizeof(name));
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  status = lj_wasm_import_ffi_symbol(host_handle, name, &host_symbol);
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  status = lj_wasmtime_guest_handle_alloc(&ctx->handles, host_symbol,
                                          &guest_symbol);
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  status = lj_wasmtime_guest_write_handle(ctx, symbol_out_ptr, guest_symbol);
  if (status != LJ_WASM_HOST_OK) {
    lj_wasmtime_guest_handle_release(&ctx->handles, guest_symbol);
  }
  return status;
}

int lj_wasmtime_guest_ffi_call(LJWasmtimeGuestContext *ctx, uint64_t cts_ptr,
                               uint64_t ct_ptr, uint64_t cc_ptr,
                               uint64_t call_ptr) {
  LJWasmFFICall call;
  uint8_t *cc_copy;
  uint64_t guest_symbol = 0;
  LJWasmHostHandle host_symbol = NULL;
  int status;

  if (ctx == NULL || cts_ptr == 0 || ct_ptr == 0) {
    return LJ_WASM_HOST_ERR;
  }
  status = lj_wasmtime_guest_read(&ctx->memory, call_ptr, &call,
                                  sizeof(call));
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  if (call.ccall_size == 0 ||
      call.ccall_size > LJ_WASMTIME_GUEST_MAX_CCALL_SIZE ||
      sizeof(guest_symbol) > call.ccall_size ||
      call.func_offset > call.ccall_size - sizeof(guest_symbol)) {
    return LJ_WASM_HOST_ERR;
  }
  cc_copy = (uint8_t *)malloc(call.ccall_size);
  if (cc_copy == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  status = lj_wasmtime_guest_read(&ctx->memory, cc_ptr, cc_copy,
                                  call.ccall_size);
  if (status != LJ_WASM_HOST_OK) {
    free(cc_copy);
    return status;
  }
  memcpy(&guest_symbol, cc_copy + call.func_offset, sizeof(guest_symbol));
  status = lj_wasmtime_guest_handle_get(&ctx->handles, guest_symbol,
                                        &host_symbol);
  if (status != LJ_WASM_HOST_OK) {
    free(cc_copy);
    return status;
  }
  memcpy(cc_copy + call.func_offset, &host_symbol, sizeof(host_symbol));
  status = lj_wasm_import_ffi_call((struct CTState *)(uintptr_t)cts_ptr,
                                   (struct CType *)(uintptr_t)ct_ptr,
                                   (struct CCallState *)cc_copy, &call);
  memcpy(cc_copy + call.func_offset, &guest_symbol, sizeof(guest_symbol));
  if (status == LJ_WASM_HOST_OK) {
    status = lj_wasmtime_guest_write(&ctx->memory, cc_ptr, cc_copy,
                                     call.ccall_size);
  }
  free(cc_copy);
  return status;
}

int lj_wasmtime_guest_jit_compile(LJWasmtimeGuestContext *ctx,
                                  uint64_t module_ptr,
                                  uint64_t handle_out_ptr) {
  LJWasmJITModule module;
  LJWasmJITModule host_module;
  uint8_t *bytes_copy;
  uint64_t bytes_ptr;
  LJWasmHostHandle host_handle = NULL;
  uint64_t guest_handle = 0;
  int status;

  if (ctx == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  status = lj_wasmtime_guest_read(&ctx->memory, module_ptr, &module,
                                  sizeof(module));
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  if (module.bytes == NULL || module.size == 0 ||
      module.size > LJ_WASMTIME_GUEST_MAX_MODULE_SIZE) {
    return LJ_WASM_HOST_ERR;
  }
  bytes_ptr = (uint64_t)(uintptr_t)module.bytes;
  bytes_copy = (uint8_t *)malloc(module.size);
  if (bytes_copy == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  status = lj_wasmtime_guest_read(&ctx->memory, bytes_ptr, bytes_copy,
                                  module.size);
  if (status != LJ_WASM_HOST_OK) {
    free(bytes_copy);
    return status;
  }
  host_module = module;
  host_module.bytes = bytes_copy;
  status = lj_wasm_import_jit_compile(&host_module, &host_handle);
  free(bytes_copy);
  if (status != LJ_WASM_HOST_OK) {
    return status;
  }
  status = lj_wasmtime_guest_handle_alloc(&ctx->handles, host_handle,
                                          &guest_handle);
  if (status != LJ_WASM_HOST_OK) {
    lj_wasm_import_jit_free(host_handle);
    return status;
  }
  status = lj_wasmtime_guest_write_handle(ctx, handle_out_ptr, guest_handle);
  if (status != LJ_WASM_HOST_OK) {
    lj_wasm_import_jit_free(host_handle);
    lj_wasmtime_guest_handle_release(&ctx->handles, guest_handle);
  }
  return status;
}

void lj_wasmtime_guest_jit_free(LJWasmtimeGuestContext *ctx,
                                uint64_t guest_handle) {
  LJWasmHostHandle host_handle = NULL;
  if (ctx == NULL || guest_handle == 0) {
    return;
  }
  if (lj_wasmtime_guest_handle_get(&ctx->handles, guest_handle,
                                   &host_handle) == LJ_WASM_HOST_OK) {
    lj_wasm_import_jit_free(host_handle);
    lj_wasmtime_guest_handle_release(&ctx->handles, guest_handle);
  }
}

int lj_wasmtime_guest_jit_enter(LJWasmtimeGuestContext *ctx,
                                uint64_t guest_handle, uint64_t lua_state,
                                uint64_t base, uint64_t exit_state,
                                uint32_t exitno) {
  LJWasmHostHandle host_handle = NULL;
  if (ctx == NULL ||
      lj_wasmtime_guest_handle_get(&ctx->handles, guest_handle,
                                   &host_handle) != LJ_WASM_HOST_OK) {
    return LJ_WASM_HOST_ERR;
  }
  return lj_wasm_import_jit_enter(host_handle, (void *)(uintptr_t)lua_state,
                                  (void *)(uintptr_t)base,
                                  (void *)(uintptr_t)exit_state, exitno);
}

int lj_wasmtime_guest_jit_patch_exit(LJWasmtimeGuestContext *ctx,
                                     uint64_t from_handle, uint32_t exitno,
                                     uint64_t to_handle) {
  LJWasmHostHandle from = NULL;
  LJWasmHostHandle to = NULL;
  if (ctx == NULL ||
      lj_wasmtime_guest_handle_get(&ctx->handles, from_handle, &from) !=
          LJ_WASM_HOST_OK ||
      lj_wasmtime_guest_handle_get(&ctx->handles, to_handle, &to) !=
          LJ_WASM_HOST_OK) {
    return LJ_WASM_HOST_ERR;
  }
  return lj_wasm_import_jit_patch_exit(from, exitno, to);
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
#if LJ_WASMTIME_ENABLE_LIBFFI
  {
    void *lib = dlopen(name, RTLD_LAZY | (global ? RTLD_GLOBAL : RTLD_LOCAL));
    if (lib == NULL) {
      return LJ_WASM_HOST_ERR;
    }
    if (lj_wasmtime_ffi_new_handle(LJ_WASMTIME_FFI_KIND_LIB, lib, NULL,
                                   handle) != LJ_WASM_HOST_OK) {
      dlclose(lib);
      return LJ_WASM_HOST_ERR;
    }
    return LJ_WASM_HOST_OK;
  }
#else
  return LJ_WASM_HOST_NYI;
#endif
}

void lj_wasm_import_ffi_unload(LJWasmHostHandle handle) {
  if (lj_wasmtime_hooks.ffi_unload != NULL) {
    lj_wasmtime_hooks.ffi_unload(lj_wasmtime_hooks.ctx, handle);
    return;
  }
#if LJ_WASMTIME_ENABLE_LIBFFI
  if (handle != NULL) {
    LJWasmtimeFFIHandle *lib =
        lj_wasmtime_ffi_handle(handle, LJ_WASMTIME_FFI_KIND_LIB);
    if (lib != NULL) {
      dlclose(lib->value);
      lj_wasmtime_ffi_invalidate_handle(lib);
    }
  }
#endif
}

int lj_wasm_import_ffi_symbol(LJWasmHostHandle handle, const char *name,
                              LJWasmHostHandle *symbol) {
  if (symbol != NULL) {
    *symbol = NULL;
  }
  if (name == NULL || symbol == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  if (lj_wasmtime_hooks.ffi_symbol != NULL) {
    return lj_wasmtime_hooks.ffi_symbol(lj_wasmtime_hooks.ctx, handle, name,
                                        symbol);
  }
#if LJ_WASMTIME_ENABLE_LIBFFI
  {
    LJWasmtimeFFIHandle *lib = NULL;
    void *lookup = RTLD_DEFAULT;
    void *addr;
    if (handle != NULL) {
      lib = lj_wasmtime_ffi_handle(handle, LJ_WASMTIME_FFI_KIND_LIB);
      if (lib == NULL) {
        return LJ_WASM_HOST_ERR;
      }
      lookup = lib->value;
    }
    dlerror();
    addr = dlsym(lookup, name);
    if (dlerror() != NULL) {
      return LJ_WASM_HOST_ERR;
    }
    return lj_wasmtime_ffi_new_handle(LJ_WASMTIME_FFI_KIND_SYMBOL, addr, lib,
                                      symbol);
  }
#else
  if (handle == NULL) {
    return LJ_WASM_HOST_ERR;
  }
  return LJ_WASM_HOST_NYI;
#endif
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
#if LJ_WASMTIME_ENABLE_LIBFFI
  return lj_wasmtime_ffi_default_call(cc, call);
#else
  return LJ_WASM_HOST_NYI;
#endif
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
