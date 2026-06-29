/*
** WebAssembly trace module builder.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_wasm_jit_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_buf.h"
#include "lj_wasm_emit.h"
#if LJ_TARGET_WASM
#include "lj_wasm_host.h"
#endif
#include "lj_wasm_jit.h"

static void wasm_sbuf_free(lua_State *L, SBuf *sb)
{
  if (sb->b)
    lj_buf_free(G(L), sb);
}

/*
** Build a minimal, valid trace module:
**
**   (module
**     (func (export "entry") (param i64 i64 i32) (result i32)
**       (i32.const -2)))
**
** The parameters are reserved for lua_State, base and exit number. Returning
** NYI lets the host compile/link path be exercised before IR lowering exists.
*/
void lj_wasm_jit_build_nyi(lua_State *L, SBuf *module)
{
  static const uint8_t entry_params[] = {
    LJ_WASM_TYPE_I64, LJ_WASM_TYPE_I64, LJ_WASM_TYPE_I32
  };
  static const uint8_t entry_results[] = { LJ_WASM_TYPE_I32 };
  SBuf type, func, exp, code, body;

  lj_buf_init(L, &type);
  lj_buf_init(L, &func);
  lj_buf_init(L, &exp);
  lj_buf_init(L, &code);
  lj_buf_init(L, &body);

  lj_wasm_module_begin(module);

  lj_wasm_putu32v(&type, 1);
  lj_wasm_putfunctype(&type, entry_params, (MSize)sizeof(entry_params),
		      entry_results, (MSize)sizeof(entry_results));
  lj_wasm_putsection(module, LJ_WASM_SECT_TYPE, &type);

  lj_wasm_putu32v(&func, 1);
  lj_wasm_putu32v(&func, 0);
  lj_wasm_putsection(module, LJ_WASM_SECT_FUNCTION, &func);

  lj_wasm_putu32v(&exp, 1);
  lj_wasm_putexport(&exp, "entry", 5, LJ_WASM_EXT_FUNC, 0);
  lj_wasm_putsection(module, LJ_WASM_SECT_EXPORT, &exp);

  lj_wasm_putu32v(&body, 0);  /* Local declaration count. */
  lj_wasm_putu8(&body, LJ_WASM_OP_I32_CONST);
  lj_wasm_puti32v(&body, LJ_WASM_JIT_STATUS_NYI);
  lj_wasm_putu8(&body, LJ_WASM_OP_END);

  lj_wasm_putu32v(&code, 1);
  lj_wasm_putfuncbody(&code, &body);
  lj_wasm_putsection(module, LJ_WASM_SECT_CODE, &code);

  wasm_sbuf_free(L, &body);
  wasm_sbuf_free(L, &code);
  wasm_sbuf_free(L, &exp);
  wasm_sbuf_free(L, &func);
  wasm_sbuf_free(L, &type);
}

#if LJ_TARGET_WASM
int lj_wasm_jit_compile_nyi(lua_State *L, uint32_t traceno,
			    LJWasmHostHandle *handle)
{
  LJWasmJITModule module;
  SBuf sb;
  int status;

  lj_buf_init(L, &sb);
  lj_wasm_jit_build_nyi(L, &sb);

  module.bytes = (const uint8_t *)sb.b;
  module.size = sbuflen(&sb);
  module.trace = traceno;
  module.entry = 0;
  module.exit = 0;
  module.flags = 0;

  status = lj_wasm_host_jit_compile(&module, handle);

  if (sb.b)
    lj_buf_free(G(L), &sb);
  return status;
}
#endif
