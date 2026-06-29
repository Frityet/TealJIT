/*
** WebAssembly trace module builder.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_WASM_JIT_H
#define _LJ_WASM_JIT_H

#include "lj_obj.h"
#include "lj_buf.h"
#include "lj_jit.h"
#if LJ_TARGET_WASM
#include "lj_wasm_host.h"
#endif

/*
** Negative values returned by a trace entry are backend statuses. Non-negative
** values are LuaJIT trace exit numbers and are interpreted as snapshot IDs by
** the exit trampoline path. NYI intentionally matches LJ_WASM_HOST_NYI.
*/
#define LJ_WASM_JIT_STATUS_NYI	(-2)

LJ_FUNC void lj_wasm_jit_build_nyi(lua_State *L, SBuf *module);
#if LJ_HASJIT
LJ_FUNC int lj_wasm_jit_build_trace(lua_State *L, const GCtrace *T,
				    SBuf *module);
#endif
#if LJ_TARGET_WASM
LJ_FUNC int lj_wasm_jit_compile_nyi(lua_State *L, uint32_t traceno,
				    LJWasmHostHandle *handle);
#if LJ_HASJIT
LJ_FUNC int lj_wasm_jit_compile_trace(lua_State *L, const GCtrace *T,
				      LJWasmHostHandle *handle);
#endif
#endif

#endif
