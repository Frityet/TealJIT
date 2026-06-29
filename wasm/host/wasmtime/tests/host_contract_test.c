#include "lj_wasmtime_host.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct TestCtx {
  int trace_handle;
  int linked_handle;
  unsigned compile_calls;
  unsigned free_calls;
  unsigned enter_calls;
  unsigned patch_calls;
  uint32_t last_trace;
  uint32_t last_flags;
  uint32_t last_exitno;
  void *last_lua_state;
  void *last_base;
} TestCtx;

static int test_jit_compile(void *ud, const LJWasmJITModule *module,
                            LJWasmHostHandle *handle) {
  TestCtx *ctx = (TestCtx *)ud;
  ctx->compile_calls++;
  ctx->last_trace = module->trace;
  ctx->last_flags = module->flags;
  *handle = &ctx->trace_handle;
  return LJ_WASM_HOST_OK;
}

static void test_jit_free(void *ud, LJWasmHostHandle handle) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->trace_handle);
  ctx->free_calls++;
}

static int test_jit_enter(void *ud, LJWasmHostHandle handle, void *lua_state,
                          void *base, uint32_t exitno) {
  TestCtx *ctx = (TestCtx *)ud;
  assert(handle == &ctx->trace_handle);
  ctx->enter_calls++;
  ctx->last_lua_state = lua_state;
  ctx->last_base = base;
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
  TestCtx ctx;
  LJWasmtimeHostHooks hooks;
  int fake_lua;
  int fake_base;

  memset(&module, 0, sizeof(module));
  module.bytes = module_bytes;
  module.size = sizeof(module_bytes);
  module.trace = 42;
  module.flags = 7;

  lj_wasmtime_host_clear_hooks();
  assert(strcmp(lj_wasmtime_host_status_name(LJ_WASM_HOST_OK), "ok") == 0);
  assert(strcmp(lj_wasmtime_host_status_name(LJ_WASM_HOST_ERR), "error") == 0);
  assert(strcmp(lj_wasmtime_host_status_name(LJ_WASM_HOST_NYI),
                "not-yet-implemented") == 0);
  assert(strcmp(lj_wasmtime_host_status_name(1234), "unknown") == 0);

  assert(lj_wasm_import_jit_compile(NULL, &handle) == LJ_WASM_HOST_ERR);
  assert(handle == NULL);
  assert(lj_wasm_import_jit_compile(&module, &handle) == LJ_WASM_HOST_NYI);
  assert(handle == NULL);
  assert(lj_wasm_import_jit_enter(NULL, &fake_lua, &fake_base, 0) ==
         LJ_WASM_HOST_ERR);
  assert(lj_wasm_import_jit_patch_exit(&fake_lua, 0, NULL) ==
         LJ_WASM_HOST_ERR);

  memset(&ctx, 0, sizeof(ctx));
  memset(&hooks, 0, sizeof(hooks));
  hooks.ctx = &ctx;
  hooks.jit_compile = test_jit_compile;
  hooks.jit_free = test_jit_free;
  hooks.jit_enter = test_jit_enter;
  hooks.jit_patch_exit = test_jit_patch_exit;
  lj_wasmtime_host_set_hooks(&hooks);

  assert(lj_wasm_import_jit_compile(&module, &handle) == LJ_WASM_HOST_OK);
  assert(handle == &ctx.trace_handle);
  assert(ctx.compile_calls == 1);
  assert(ctx.last_trace == 42);
  assert(ctx.last_flags == 7);

  assert(lj_wasm_import_jit_enter(handle, &fake_lua, &fake_base, 3) ==
         LJ_WASM_HOST_OK);
  assert(ctx.enter_calls == 1);
  assert(ctx.last_lua_state == &fake_lua);
  assert(ctx.last_base == &fake_base);
  assert(ctx.last_exitno == 3);

  assert(lj_wasm_import_jit_patch_exit(handle, 5, &ctx.linked_handle) ==
         LJ_WASM_HOST_OK);
  assert(ctx.patch_calls == 1);
  assert(ctx.last_exitno == 5);

  lj_wasm_import_jit_free(handle);
  assert(ctx.free_calls == 1);

  lj_wasmtime_host_clear_hooks();
  assert(lj_wasm_import_jit_enter(handle, &fake_lua, &fake_base, 0) ==
         LJ_WASM_HOST_NYI);
  return 0;
}
