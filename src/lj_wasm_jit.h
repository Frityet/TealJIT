/*
** WebAssembly trace module builder.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_WASM_JIT_H
#define _LJ_WASM_JIT_H

#include "lj_obj.h"
#include "lj_buf.h"

/*
** Placeholder status returned by trace modules before real IR lowering exists.
** This intentionally matches LJ_WASM_HOST_NYI in the host contract.
*/
#define LJ_WASM_JIT_STATUS_NYI	(-2)

LJ_FUNC void lj_wasm_jit_build_nyi(lua_State *L, SBuf *module);

#endif
