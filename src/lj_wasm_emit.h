/*
** WebAssembly binary emitter.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_WASM_EMIT_H
#define _LJ_WASM_EMIT_H

#include "lj_obj.h"
#include "lj_buf.h"

/* -- Binary format constants -------------------------------------------- */

typedef enum LJWasmSectionID {
  LJ_WASM_SECT_CUSTOM = 0,
  LJ_WASM_SECT_TYPE = 1,
  LJ_WASM_SECT_IMPORT = 2,
  LJ_WASM_SECT_FUNCTION = 3,
  LJ_WASM_SECT_TABLE = 4,
  LJ_WASM_SECT_MEMORY = 5,
  LJ_WASM_SECT_GLOBAL = 6,
  LJ_WASM_SECT_EXPORT = 7,
  LJ_WASM_SECT_START = 8,
  LJ_WASM_SECT_ELEMENT = 9,
  LJ_WASM_SECT_CODE = 10,
  LJ_WASM_SECT_DATA = 11,
  LJ_WASM_SECT_DATACOUNT = 12
} LJWasmSectionID;

typedef enum LJWasmExternalKind {
  LJ_WASM_EXT_FUNC = 0,
  LJ_WASM_EXT_TABLE = 1,
  LJ_WASM_EXT_MEMORY = 2,
  LJ_WASM_EXT_GLOBAL = 3
} LJWasmExternalKind;

typedef enum LJWasmValType {
  LJ_WASM_TYPE_I32 = 0x7f,
  LJ_WASM_TYPE_I64 = 0x7e,
  LJ_WASM_TYPE_F32 = 0x7d,
  LJ_WASM_TYPE_F64 = 0x7c,
  LJ_WASM_TYPE_FUNCREF = 0x70,
  LJ_WASM_TYPE_EXTERNREF = 0x6f
} LJWasmValType;

typedef enum LJWasmOpcode {
  LJ_WASM_OP_UNREACHABLE = 0x00,
  LJ_WASM_OP_NOP = 0x01,
  LJ_WASM_OP_BLOCK = 0x02,
  LJ_WASM_OP_LOOP = 0x03,
  LJ_WASM_OP_IF = 0x04,
  LJ_WASM_OP_ELSE = 0x05,
  LJ_WASM_OP_END = 0x0b,
  LJ_WASM_OP_BR = 0x0c,
  LJ_WASM_OP_BR_IF = 0x0d,
  LJ_WASM_OP_RETURN = 0x0f,
  LJ_WASM_OP_CALL = 0x10,
  LJ_WASM_OP_DROP = 0x1a,
  LJ_WASM_OP_LOCAL_GET = 0x20,
  LJ_WASM_OP_LOCAL_SET = 0x21,
  LJ_WASM_OP_LOCAL_TEE = 0x22,
  LJ_WASM_OP_GLOBAL_GET = 0x23,
  LJ_WASM_OP_GLOBAL_SET = 0x24,
  LJ_WASM_OP_I32_LOAD = 0x28,
  LJ_WASM_OP_I64_LOAD = 0x29,
  LJ_WASM_OP_F64_LOAD = 0x2b,
  LJ_WASM_OP_F64_STORE = 0x39,
  LJ_WASM_OP_I32_CONST = 0x41,
  LJ_WASM_OP_I64_CONST = 0x42,
  LJ_WASM_OP_F32_CONST = 0x43,
  LJ_WASM_OP_F64_CONST = 0x44,
  LJ_WASM_OP_I32_EQZ = 0x45,
  LJ_WASM_OP_I32_EQ = 0x46,
  LJ_WASM_OP_I32_NE = 0x47,
  LJ_WASM_OP_I32_LT_S = 0x48,
  LJ_WASM_OP_I32_GT_S = 0x4a,
  LJ_WASM_OP_I32_LE_S = 0x4c,
  LJ_WASM_OP_I32_GE_S = 0x4e,
  LJ_WASM_OP_I32_GE_U = 0x4f,
  LJ_WASM_OP_I64_EQ = 0x51,
  LJ_WASM_OP_I64_LT_U = 0x54,
  LJ_WASM_OP_F64_EQ = 0x61,
  LJ_WASM_OP_F64_NE = 0x62,
  LJ_WASM_OP_F64_LT = 0x63,
  LJ_WASM_OP_F64_GT = 0x64,
  LJ_WASM_OP_F64_LE = 0x65,
  LJ_WASM_OP_F64_GE = 0x66,
  LJ_WASM_OP_I32_ADD = 0x6a,
  LJ_WASM_OP_I32_SUB = 0x6b,
  LJ_WASM_OP_I32_MUL = 0x6c,
  LJ_WASM_OP_I32_REM_S = 0x6f,
  LJ_WASM_OP_I64_ADD = 0x7c,
  LJ_WASM_OP_I64_AND = 0x83,
  LJ_WASM_OP_I64_SHL = 0x86,
  LJ_WASM_OP_I64_SHR_U = 0x88,
  LJ_WASM_OP_F64_ADD = 0xa0,
  LJ_WASM_OP_F64_SUB = 0xa1,
  LJ_WASM_OP_F64_MUL = 0xa2,
  LJ_WASM_OP_F64_DIV = 0xa3,
  LJ_WASM_OP_I32_WRAP_I64 = 0xa7,
  LJ_WASM_OP_I32_TRUNC_F64_S = 0xaa,
  LJ_WASM_OP_I64_EXTEND_I32_U = 0xad,
  LJ_WASM_OP_F64_CONVERT_I32_S = 0xb7,
  LJ_WASM_OP_F64_REINTERPRET_I64 = 0xbf
} LJWasmOpcode;

#define LJ_WASM_BLOCKTYPE_EMPTY	0x40

/* -- Low-level writes ---------------------------------------------------- */

LJ_FUNC void lj_wasm_module_begin(SBuf *sb);
LJ_FUNC void lj_wasm_putu8(SBuf *sb, uint8_t x);
LJ_FUNC void lj_wasm_putu32v(SBuf *sb, uint32_t x);
LJ_FUNC void lj_wasm_putu64v(SBuf *sb, uint64_t x);
LJ_FUNC void lj_wasm_puti32v(SBuf *sb, int32_t x);
LJ_FUNC void lj_wasm_puti64v(SBuf *sb, int64_t x);
LJ_FUNC void lj_wasm_putbytes(SBuf *sb, const void *p, MSize len);
LJ_FUNC void lj_wasm_putname(SBuf *sb, const char *name, MSize len);

/* -- Structured writes --------------------------------------------------- */

LJ_FUNC void lj_wasm_putsection(SBuf *sb, uint8_t id, const SBuf *payload);
LJ_FUNC void lj_wasm_putfunctype(SBuf *sb, const uint8_t *params,
				 MSize nparams, const uint8_t *results,
				 MSize nresults);
LJ_FUNC void lj_wasm_putimport_func(SBuf *sb, const char *module,
				    MSize module_len, const char *name,
				    MSize name_len, uint32_t typeidx);
LJ_FUNC void lj_wasm_putimport_memory(SBuf *sb, const char *module,
				      MSize module_len, const char *name,
				      MSize name_len, uint64_t min,
				      uint64_t max, int hasmax,
				      int is64);
LJ_FUNC void lj_wasm_putexport(SBuf *sb, const char *name, MSize name_len,
			       uint8_t kind, uint32_t idx);
LJ_FUNC void lj_wasm_putlimits(SBuf *sb, uint64_t min, uint64_t max,
			       int hasmax, int is64);
LJ_FUNC void lj_wasm_putmemarg(SBuf *sb, uint32_t align, uint64_t ofs);
LJ_FUNC void lj_wasm_putfuncbody(SBuf *sb, const SBuf *body);

#endif
