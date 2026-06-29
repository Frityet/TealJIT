#include "lj_wasmtime_host.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef LJ_WASMTIME_TEST_LIBM
#define LJ_WASMTIME_TEST_LIBM "libm.so.6"
#endif

static double load_f64(const void *base, uint32_t offset) {
  double value;
  memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
  return value;
}

static void store_f64(void *base, uint32_t offset, double value) {
  memcpy((uint8_t *)base + offset, &value, sizeof(value));
}

#if LJ_WASMTIME_ENABLE_LIBFFI
static int32_t load_i32(const void *base, uint32_t offset) {
  int32_t value;
  memcpy(&value, (const uint8_t *)base + offset, sizeof(value));
  return value;
}

static void store_i32(void *base, uint32_t offset, int32_t value) {
  memcpy((uint8_t *)base + offset, &value, sizeof(value));
}

static void store_ptr(void *base, uint32_t offset, void *value) {
  memcpy((uint8_t *)base + offset, &value, sizeof(value));
}
#endif

typedef struct TestCtx {
  int trace_handle;
  int linked_handle;
  int ffi_handle;
  int ffi_symbol;
  int callback_handle;
  unsigned ffi_load_calls;
  unsigned ffi_unload_calls;
  unsigned ffi_symbol_calls;
  unsigned ffi_call_calls;
  unsigned callback_new_calls;
  unsigned callback_slot_calls;
  unsigned callback_free_calls;
  unsigned compile_calls;
  unsigned free_calls;
  unsigned enter_calls;
  unsigned patch_calls;
  int last_global;
  uint32_t last_slot;
  uint32_t last_ctypeid;
  uint32_t last_trace;
  uint32_t last_flags;
  uint64_t last_memory_min;
  uint64_t last_memory_max;
  uint32_t last_memory_flags;
  uint32_t last_exitno;
  void *last_lua_state;
  void *last_base;
  void *last_exit_state;
} TestCtx;

static int test_ffi_load(void *ud, const char *name, int global,
                         LJWasmHostHandle *handle) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(strcmp(name, "libm") == 0);
  ctx->ffi_load_calls++;
  ctx->last_global = global;
  *handle = &ctx->ffi_handle;
  return LJ_WASM_HOST_OK;
}

static void test_ffi_unload(void *ud, LJWasmHostHandle handle) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->ffi_handle);
  ctx->ffi_unload_calls++;
}

static int test_ffi_symbol(void *ud, LJWasmHostHandle handle, const char *name,
                           LJWasmHostHandle *symbol) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->ffi_handle);
  assert(strcmp(name, "sin") == 0);
  ctx->ffi_symbol_calls++;
  *symbol = &ctx->ffi_symbol;
  return LJ_WASM_HOST_OK;
}

static int test_ffi_call(void *ud, struct CTState *cts, struct CType *ct,
                         struct CCallState *cc,
                         const LJWasmFFICall *call) {
  TestCtx *ctx = (TestCtx *)ud;
  double lhs, rhs;
  assert(cts != NULL);
  assert(ct != NULL);
  assert(cc != NULL);
  assert(call != NULL);
  assert(call->abi_version == LJ_WASM_FFI_CALL_ABI_VERSION);
  assert(call->ccall_size == 128);
  assert(call->sig.ctypeid == 1234);
  assert(call->sig.nargs == 2);
  assert(call->sig.rettype == LJ_WASM_SCALAR_F64);
  assert((call->sig.flags & LJ_WASM_FFI_SIG_F_UNSUPPORTED) == 0);
  assert(call->ret.type == LJ_WASM_SCALAR_F64);
  assert(call->ret.loc == LJ_WASM_FFI_LOC_FPR);
  assert(call->ret.size == sizeof(double));
  assert(call->args[0].type == LJ_WASM_SCALAR_F64);
  assert(call->args[0].loc == LJ_WASM_FFI_LOC_FPR);
  assert(call->args[0].size == sizeof(double));
  assert(call->args[1].type == LJ_WASM_SCALAR_F64);
  assert(call->args[1].loc == LJ_WASM_FFI_LOC_FPR);
  assert(call->args[1].size == sizeof(double));
  lhs = load_f64(cc, call->args[0].offset);
  rhs = load_f64(cc, call->args[1].offset);
  store_f64(cc, call->ret.offset, lhs + rhs);
  ctx->ffi_call_calls++;
  return LJ_WASM_HOST_OK;
}

static int test_callback_new(void *ud, uint32_t slot, uint32_t ctypeid,
                             LJWasmHostHandle *handle) {
  TestCtx *ctx = (TestCtx *)ud;
  ctx->callback_new_calls++;
  ctx->last_slot = slot;
  ctx->last_ctypeid = ctypeid;
  *handle = &ctx->callback_handle;
  return LJ_WASM_HOST_OK;
}

static int test_callback_slot(void *ud, LJWasmHostHandle handle,
                              uint32_t *slot) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->callback_handle);
  ctx->callback_slot_calls++;
  *slot = ctx->last_slot;
  return LJ_WASM_HOST_OK;
}

static void test_callback_free(void *ud, LJWasmHostHandle handle) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->callback_handle);
  ctx->callback_free_calls++;
}

static int test_jit_compile(void *ud, const LJWasmJITModule *module,
                            LJWasmHostHandle *handle) {
  TestCtx *ctx = (TestCtx *)ud;
  ctx->compile_calls++;
  ctx->last_trace = module->trace;
  ctx->last_flags = module->flags;
  ctx->last_memory_min = module->memory_min;
  ctx->last_memory_max = module->memory_max;
  ctx->last_memory_flags = module->memory_flags;
  *handle = &ctx->trace_handle;
  return LJ_WASM_HOST_OK;
}

static void test_jit_free(void *ud, LJWasmHostHandle handle) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->trace_handle);
  ctx->free_calls++;
}

static int test_jit_enter(void *ud, LJWasmHostHandle handle, void *lua_state,
                          void *base, void *exit_state, uint32_t exitno) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->trace_handle);
  ctx->enter_calls++;
  ctx->last_lua_state = lua_state;
  ctx->last_base = base;
  ctx->last_exit_state = exit_state;
  ctx->last_exitno = exitno;
  return LJ_WASM_HOST_OK;
}

static int test_jit_patch_exit(void *ud, LJWasmHostHandle from,
                               uint32_t exitno, LJWasmHostHandle to) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(from == &ctx->trace_handle);
  assert(to == &ctx->linked_handle);
  ctx->patch_calls++;
  ctx->last_exitno = exitno;
  return LJ_WASM_HOST_OK;
}

int main(void) {
  static const uint8_t module_bytes[] = {0x00, 0x61, 0x73, 0x6d};
  LJWasmJITModule module;
  LJWasmHostHandle handle = (LJWasmHostHandle)1;
  LJWasmHostHandle symbol = (LJWasmHostHandle)1;
  uint32_t slot = 0;
  TestCtx ctx;
  LJWasmtimeHostHooks hooks;
  int fake_lua;
  int fake_base;
  int fake_exit_state;
  int fake_cts;
  int fake_ct;
  uint8_t fake_cc[128];
  LJWasmFFICall fake_call;
  LJWasmJITModule bad_module;
  uint8_t guest_bytes[32];
  LJWasmtimeGuestMemory guest_memory;
  uint32_t guest_word;
  char guest_name[8];
  void *guest_host_ptr;

  assert(sizeof(LJWasmFFISig) == 8);
  assert(offsetof(LJWasmFFICall, sig) == 0);
  assert(offsetof(LJWasmFFICall, ret) >
         offsetof(LJWasmFFICall, reserved));

  memset(guest_bytes, 0, sizeof(guest_bytes));
  guest_memory.data = guest_bytes;
  guest_memory.size = sizeof(guest_bytes);
  guest_word = 0x12345678u;
  assert(lj_wasmtime_guest_write(&guest_memory, 4, &guest_word,
                                 sizeof(guest_word)) == LJ_WASM_HOST_OK);
  guest_word = 0;
  assert(lj_wasmtime_guest_read(&guest_memory, 4, &guest_word,
                                sizeof(guest_word)) == LJ_WASM_HOST_OK);
  assert(guest_word == 0x12345678u);
  assert(lj_wasmtime_guest_ptr(&guest_memory, guest_memory.size, 0,
                               &guest_host_ptr) == LJ_WASM_HOST_OK);
  assert(lj_wasmtime_guest_ptr(&guest_memory, guest_memory.size, 1,
                               &guest_host_ptr) == LJ_WASM_HOST_ERR);
  assert(guest_host_ptr == NULL);
  assert(lj_wasmtime_guest_ptr(&guest_memory, UINT64_MAX, 1,
                               &guest_host_ptr) == LJ_WASM_HOST_ERR);
  assert(guest_host_ptr == NULL);
  memcpy(guest_bytes + 12, "libm", 5);
  assert(lj_wasmtime_guest_read_cstr(&guest_memory, 12, guest_name,
                                     sizeof(guest_name)) == LJ_WASM_HOST_OK);
  assert(strcmp(guest_name, "libm") == 0);
  assert(lj_wasmtime_guest_read_cstr(&guest_memory, 12, guest_name, 4) ==
         LJ_WASM_HOST_ERR);
  memset(guest_bytes + 20, 'x', sizeof(guest_bytes) - 20);
  assert(lj_wasmtime_guest_read_cstr(&guest_memory, 20, guest_name,
                                     sizeof(guest_name)) == LJ_WASM_HOST_ERR);

  memset(&module, 0, sizeof(module));
  module.bytes = module_bytes;
  module.size = sizeof(module_bytes);
  module.trace = 42;
  module.flags = LJ_WASM_JIT_F_IR_LOWERED | LJ_WASM_JIT_F_IMPORT_ENV_MEMORY;
  module.memory_min = 1;
  module.memory_max = 0;
  module.memory_flags = LJ_WASM_JIT_MEMORY_F_64;

  lj_wasmtime_host_clear_hooks();
  assert(strcmp(lj_wasmtime_host_status_name(LJ_WASM_HOST_OK), "ok") == 0);
  assert(strcmp(lj_wasmtime_host_status_name(LJ_WASM_HOST_ERR), "error") == 0);
  assert(strcmp(lj_wasmtime_host_status_name(LJ_WASM_HOST_NYI),
                "not-yet-implemented") == 0);
  assert(strcmp(lj_wasmtime_host_status_name(1234), "unknown") == 0);
  assert(lj_wasmtime_host_validate_jit_module(&module) == LJ_WASM_HOST_OK);

  bad_module = module;
  bad_module.flags &= ~LJ_WASM_JIT_F_IMPORT_ENV_MEMORY;
  assert(lj_wasmtime_host_validate_jit_module(&bad_module) ==
         LJ_WASM_HOST_ERR);
  assert(lj_wasm_import_jit_compile(&bad_module, &handle) ==
         LJ_WASM_HOST_ERR);
  assert(handle == NULL);

  bad_module = module;
  bad_module.memory_flags = 0;
  assert(lj_wasmtime_host_validate_jit_module(&bad_module) ==
         LJ_WASM_HOST_ERR);

  bad_module = module;
  bad_module.memory_flags =
      LJ_WASM_JIT_MEMORY_F_64 | LJ_WASM_JIT_MEMORY_F_HAS_MAX;
  bad_module.memory_max = 0;
  assert(lj_wasmtime_host_validate_jit_module(&bad_module) ==
         LJ_WASM_HOST_ERR);

  bad_module = module;
  bad_module.reserved = 1;
  assert(lj_wasmtime_host_validate_jit_module(&bad_module) ==
         LJ_WASM_HOST_ERR);

  bad_module = module;
  bad_module.flags = 0;
  bad_module.memory_min = 0;
  bad_module.memory_max = 0;
  bad_module.memory_flags = 0;
  assert(lj_wasmtime_host_validate_jit_module(&bad_module) ==
         LJ_WASM_HOST_OK);

  assert(lj_wasm_import_jit_compile(NULL, &handle) == LJ_WASM_HOST_ERR);
  assert(handle == NULL);
  assert(lj_wasm_import_jit_compile(&module, &handle) == LJ_WASM_HOST_NYI);
  assert(handle == NULL);
  assert(lj_wasm_import_jit_enter(NULL, &fake_lua, &fake_base,
                                  &fake_exit_state, 0) ==
         LJ_WASM_HOST_ERR);
  assert(lj_wasm_import_jit_patch_exit(&fake_lua, 0, NULL) ==
         LJ_WASM_HOST_ERR);
  assert(lj_wasm_import_ffi_load(NULL, 0, &handle) == LJ_WASM_HOST_ERR);
  assert(handle == NULL);
#if !LJ_WASMTIME_ENABLE_LIBFFI
  assert(lj_wasm_import_ffi_load("libm", 1, &handle) == LJ_WASM_HOST_NYI);
  assert(handle == NULL);
#endif
  assert(lj_wasm_import_ffi_symbol(NULL, "__lj_wasm_missing_symbol__",
                                   &symbol) == LJ_WASM_HOST_ERR);
  assert(symbol == NULL);
  memset(&fake_call, 0, sizeof(fake_call));
  assert(lj_wasm_import_ffi_call(NULL, (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) ==
         LJ_WASM_HOST_ERR);
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 NULL) == LJ_WASM_HOST_ERR);
  assert(lj_wasm_import_ffi_callback_new(7, 9, &handle) ==
         LJ_WASM_HOST_NYI);
  assert(handle == NULL);
  assert(lj_wasm_import_ffi_callback_slot(NULL, &slot) == LJ_WASM_HOST_ERR);
  assert(slot == UINT32_MAX);

  memset(&ctx, 0, sizeof(ctx));
  memset(&hooks, 0, sizeof(hooks));
  hooks.ctx = &ctx;
  hooks.ffi_load = test_ffi_load;
  hooks.ffi_unload = test_ffi_unload;
  hooks.ffi_symbol = test_ffi_symbol;
  hooks.ffi_call = test_ffi_call;
  hooks.ffi_callback_new = test_callback_new;
  hooks.ffi_callback_slot = test_callback_slot;
  hooks.ffi_callback_free = test_callback_free;
  hooks.jit_compile = test_jit_compile;
  hooks.jit_free = test_jit_free;
  hooks.jit_enter = test_jit_enter;
  hooks.jit_patch_exit = test_jit_patch_exit;
  lj_wasmtime_host_set_hooks(&hooks);

  assert(lj_wasm_import_ffi_load("libm", 1, &handle) == LJ_WASM_HOST_OK);
  assert(handle == &ctx.ffi_handle);
  assert(ctx.ffi_load_calls == 1);
  assert(ctx.last_global == 1);

  assert(lj_wasm_import_ffi_symbol(handle, "sin", &symbol) == LJ_WASM_HOST_OK);
  assert(symbol == &ctx.ffi_symbol);
  assert(ctx.ffi_symbol_calls == 1);

  memset(fake_cc, 0, sizeof(fake_cc));
  memset(&fake_call, 0, sizeof(fake_call));
  fake_call.sig.ctypeid = 1234;
  fake_call.sig.nargs = 2;
  fake_call.sig.rettype = LJ_WASM_SCALAR_F64;
  fake_call.abi_version = LJ_WASM_FFI_CALL_ABI_VERSION;
  fake_call.ccall_size = sizeof(fake_cc);
  fake_call.func_offset = 0;
  fake_call.ret.type = LJ_WASM_SCALAR_F64;
  fake_call.ret.loc = LJ_WASM_FFI_LOC_FPR;
  fake_call.ret.offset = 32;
  fake_call.ret.size = sizeof(double);
  fake_call.args[0].type = LJ_WASM_SCALAR_F64;
  fake_call.args[0].loc = LJ_WASM_FFI_LOC_FPR;
  fake_call.args[0].offset = 16;
  fake_call.args[0].size = sizeof(double);
  fake_call.args[1].type = LJ_WASM_SCALAR_F64;
  fake_call.args[1].loc = LJ_WASM_FFI_LOC_FPR;
  fake_call.args[1].offset = 24;
  fake_call.args[1].size = sizeof(double);
  store_f64(fake_cc, fake_call.args[0].offset, 1.25);
  store_f64(fake_cc, fake_call.args[1].offset, 2.5);
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) ==
         LJ_WASM_HOST_OK);
  assert(ctx.ffi_call_calls == 1);
  assert(load_f64(fake_cc, fake_call.ret.offset) == 3.75);

  assert(lj_wasm_import_ffi_callback_new(7, 9, &handle) == LJ_WASM_HOST_OK);
  assert(handle == &ctx.callback_handle);
  assert(ctx.callback_new_calls == 1);
  assert(ctx.last_slot == 7);
  assert(ctx.last_ctypeid == 9);

  slot = 0;
  assert(lj_wasm_import_ffi_callback_slot(handle, &slot) == LJ_WASM_HOST_OK);
  assert(slot == 7);
  assert(ctx.callback_slot_calls == 1);

  lj_wasm_import_ffi_callback_free(handle);
  assert(ctx.callback_free_calls == 1);
  lj_wasm_import_ffi_unload(&ctx.ffi_handle);
  assert(ctx.ffi_unload_calls == 1);

  assert(lj_wasm_import_jit_compile(&module, &handle) == LJ_WASM_HOST_OK);
  assert(handle == &ctx.trace_handle);
  assert(ctx.compile_calls == 1);
  assert(ctx.last_trace == 42);
  assert(ctx.last_flags == (LJ_WASM_JIT_F_IR_LOWERED |
                            LJ_WASM_JIT_F_IMPORT_ENV_MEMORY));
  assert(ctx.last_memory_min == 1);
  assert(ctx.last_memory_max == 0);
  assert(ctx.last_memory_flags == LJ_WASM_JIT_MEMORY_F_64);

  assert(lj_wasm_import_jit_enter(handle, &fake_lua, &fake_base,
                                  &fake_exit_state, 3) ==
         LJ_WASM_HOST_OK);
  assert(ctx.enter_calls == 1);
  assert(ctx.last_lua_state == &fake_lua);
  assert(ctx.last_base == &fake_base);
  assert(ctx.last_exit_state == &fake_exit_state);
  assert(ctx.last_exitno == 3);

  assert(lj_wasm_import_jit_patch_exit(handle, 5, &ctx.linked_handle) ==
         LJ_WASM_HOST_OK);
  assert(ctx.patch_calls == 1);
  assert(ctx.last_exitno == 5);

  lj_wasm_import_jit_free(handle);
  assert(ctx.free_calls == 1);

  lj_wasmtime_host_clear_hooks();
#if LJ_WASMTIME_ENABLE_LIBFFI
  handle = NULL;
  symbol = NULL;
  assert(lj_wasm_import_ffi_load(LJ_WASMTIME_TEST_LIBM, 0, &handle) ==
         LJ_WASM_HOST_OK);
  assert(handle != NULL);
  assert(lj_wasm_import_ffi_symbol(handle, "cos", &symbol) == LJ_WASM_HOST_OK);
  assert(symbol != NULL);

  memset(fake_cc, 0, sizeof(fake_cc));
  memset(&fake_call, 0, sizeof(fake_call));
  fake_call.sig.nargs = 1;
  fake_call.sig.rettype = LJ_WASM_SCALAR_F64;
  fake_call.abi_version = LJ_WASM_FFI_CALL_ABI_VERSION;
  fake_call.ccall_size = sizeof(fake_cc);
  fake_call.func_offset = 0;
  fake_call.ret.type = LJ_WASM_SCALAR_F64;
  fake_call.ret.loc = LJ_WASM_FFI_LOC_FPR;
  fake_call.ret.offset = 32;
  fake_call.ret.size = sizeof(double);
  fake_call.args[0].type = LJ_WASM_SCALAR_F64;
  fake_call.args[0].loc = LJ_WASM_FFI_LOC_FPR;
  fake_call.args[0].offset = 16;
  fake_call.args[0].size = sizeof(double);
  store_ptr(fake_cc, fake_call.func_offset, symbol);
  store_f64(fake_cc, fake_call.args[0].offset, 0.0);
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) == LJ_WASM_HOST_OK);
  assert(load_f64(fake_cc, fake_call.ret.offset) == 1.0);
  lj_wasm_import_ffi_unload(handle);
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) == LJ_WASM_HOST_ERR);
  handle = NULL;

  assert(lj_wasm_import_ffi_symbol(NULL, "abs", &symbol) == LJ_WASM_HOST_OK);
  assert(symbol != NULL);
  memset(fake_cc, 0, sizeof(fake_cc));
  memset(&fake_call, 0, sizeof(fake_call));
  fake_call.sig.nargs = 1;
  fake_call.sig.rettype = LJ_WASM_SCALAR_I32;
  fake_call.abi_version = LJ_WASM_FFI_CALL_ABI_VERSION;
  fake_call.ccall_size = sizeof(fake_cc);
  fake_call.func_offset = 0;
  fake_call.ret.type = LJ_WASM_SCALAR_I32;
  fake_call.ret.loc = LJ_WASM_FFI_LOC_GPR;
  fake_call.ret.offset = 40;
  fake_call.ret.size = sizeof(int32_t);
  fake_call.args[0].type = LJ_WASM_SCALAR_I32;
  fake_call.args[0].loc = LJ_WASM_FFI_LOC_GPR;
  fake_call.args[0].offset = 16;
  fake_call.args[0].size = sizeof(int32_t);
  store_ptr(fake_cc, fake_call.func_offset, symbol);
  store_i32(fake_cc, fake_call.args[0].offset, -7);
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) == LJ_WASM_HOST_OK);
  assert(load_i32(fake_cc, fake_call.ret.offset) == 7);

  fake_call.sig.flags = LJ_WASM_FFI_SIG_F_UNSUPPORTED;
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) == LJ_WASM_HOST_NYI);
  fake_call.sig.flags = 0;
  fake_call.args[0].type = LJ_WASM_SCALAR_PTR;
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) == LJ_WASM_HOST_NYI);
  fake_call.args[0].type = LJ_WASM_SCALAR_I32;
  fake_call.reserved = 1;
  assert(lj_wasm_import_ffi_call((struct CTState *)&fake_cts,
                                 (struct CType *)&fake_ct,
                                 (struct CCallState *)fake_cc,
                                 &fake_call) == LJ_WASM_HOST_ERR);
#endif
  handle = &fake_lua;
  assert(lj_wasm_import_jit_enter(handle, &fake_lua, &fake_base,
                                  &fake_exit_state, 0) ==
         LJ_WASM_HOST_NYI);
  return 0;
}
