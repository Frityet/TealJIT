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

static void wasm_putf64(SBuf *sb, lua_Number n)
{
  union {
    lua_Number n;
    uint8_t b[8];
  } u;
  u.n = n;
  lj_wasm_putbytes(sb, u.b, 8);
}

static void wasm_build_module(lua_State *L, SBuf *module, const SBuf *body,
			      int import_memory)
{
  static const uint8_t entry_params[] = {
    LJ_WASM_TYPE_I64, LJ_WASM_TYPE_I64, LJ_WASM_TYPE_I32
  };
  static const uint8_t entry_results[] = { LJ_WASM_TYPE_I32 };
  SBuf type, import, func, exp, code;

  lj_buf_init(L, &type);
  lj_buf_init(L, &import);
  lj_buf_init(L, &func);
  lj_buf_init(L, &exp);
  lj_buf_init(L, &code);

  lj_wasm_module_begin(module);

  lj_wasm_putu32v(&type, 1);
  lj_wasm_putfunctype(&type, entry_params, (MSize)sizeof(entry_params),
		      entry_results, (MSize)sizeof(entry_results));
  lj_wasm_putsection(module, LJ_WASM_SECT_TYPE, &type);

  if (import_memory) {
    lj_wasm_putu32v(&import, 1);
    lj_wasm_putimport_memory(&import, "env", 3, "memory", 6, 1, 0, 0, 1);
    lj_wasm_putsection(module, LJ_WASM_SECT_IMPORT, &import);
  }

  lj_wasm_putu32v(&func, 1);
  lj_wasm_putu32v(&func, 0);
  lj_wasm_putsection(module, LJ_WASM_SECT_FUNCTION, &func);

  lj_wasm_putu32v(&exp, 1);
  lj_wasm_putexport(&exp, "entry", 5, LJ_WASM_EXT_FUNC, 0);
  lj_wasm_putsection(module, LJ_WASM_SECT_EXPORT, &exp);

  lj_wasm_putu32v(&code, 1);
  lj_wasm_putfuncbody(&code, body);
  lj_wasm_putsection(module, LJ_WASM_SECT_CODE, &code);

  wasm_sbuf_free(L, &code);
  wasm_sbuf_free(L, &exp);
  wasm_sbuf_free(L, &func);
  wasm_sbuf_free(L, &import);
  wasm_sbuf_free(L, &type);
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
  SBuf body;
  lj_buf_init(L, &body);

  lj_wasm_putu32v(&body, 0);  /* Local declaration count. */
  lj_wasm_putu8(&body, LJ_WASM_OP_I32_CONST);
  lj_wasm_puti32v(&body, LJ_WASM_JIT_STATUS_NYI);
  lj_wasm_putu8(&body, LJ_WASM_OP_END);

  wasm_build_module(L, module, &body, 0);
  wasm_sbuf_free(L, &body);
}

#if LJ_HASJIT

typedef struct WasmTraceCtx {
  const GCtrace *T;
  SBuf *body;
  uint32_t tmp64;
} WasmTraceCtx;

static uint8_t wasm_ir_valtype(IRIns *ir)
{
  if (irt_isfp(ir->t))
    return LJ_WASM_TYPE_F64;
  else if (irt_is64(ir->t) || irt_isaddr(ir->t))
    return LJ_WASM_TYPE_I64;
  else
    return LJ_WASM_TYPE_I32;
}

static uint32_t wasm_ir_local(IRRef ref)
{
  return 3u + (uint32_t)(ref - REF_FIRST);
}

static uint32_t wasm_tmp64_local(const GCtrace *T)
{
  return 3u + (uint32_t)(T->nins - REF_FIRST);
}

static uint64_t wasm_gc64_tag(uint32_t itype)
{
  return (uint64_t)(itype & 0x1ffffu);
}

static int wasm_ref_isknown(const GCtrace *T, IRRef ref)
{
  if (ref == REF_NIL || ref == REF_FALSE || ref == REF_TRUE)
    return 1;
  if (irref_isk(ref))
    return ref >= T->nk && ref < REF_BASE;
  return ref >= REF_FIRST && ref < T->nins;
}

static uint8_t wasm_ref_valtype(const GCtrace *T, IRRef ref)
{
  if (ref == REF_NIL || ref == REF_FALSE || ref == REF_TRUE)
    return LJ_WASM_TYPE_I32;
  return wasm_ir_valtype(&T->ir[ref]);
}

static int wasm_ref_can_type(const GCtrace *T, IRRef ref, uint8_t want)
{
  if (!wasm_ref_isknown(T, ref))
    return 0;
  if (!irref_isk(ref))
    return wasm_ref_valtype(T, ref) == want;
  if (ref == REF_NIL || ref == REF_FALSE || ref == REF_TRUE)
    return want == LJ_WASM_TYPE_I32;
  switch ((IROp)T->ir[ref].o) {
  case IR_KINT:
    return want == LJ_WASM_TYPE_I32 || want == LJ_WASM_TYPE_I64 ||
	   want == LJ_WASM_TYPE_F64;
  case IR_KNUM:
    return want == LJ_WASM_TYPE_F64;
  case IR_KINT64:
    return want == LJ_WASM_TYPE_I64;
  case IR_KNULL:
    return want == LJ_WASM_TYPE_I32 || want == LJ_WASM_TYPE_I64;
  default:
    return 0;
  }
}

static uint8_t wasm_ref_fixed_type(const GCtrace *T, IRRef ref)
{
  if (!irref_isk(ref))
    return wasm_ref_valtype(T, ref);
  return 0;
}

static int wasm_emit_ref(WasmTraceCtx *ctx, IRRef ref, uint8_t want)
{
  SBuf *body = ctx->body;
  const GCtrace *T = ctx->T;
  if (!wasm_ref_isknown(T, ref))
    return 0;
  if (!irref_isk(ref)) {
    if (wasm_ref_valtype(T, ref) != want)
      return 0;
    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_GET);
    lj_wasm_putu32v(body, wasm_ir_local(ref));
    return 1;
  }
  if (ref == REF_NIL || ref == REF_FALSE || ref == REF_TRUE) {
    if (want != LJ_WASM_TYPE_I32)
      return 0;
    lj_wasm_putu8(body, LJ_WASM_OP_I32_CONST);
    lj_wasm_puti32v(body, ref == REF_TRUE);
    return 1;
  }
  {
    IRIns *ir = &T->ir[ref];
    switch ((IROp)ir->o) {
    case IR_KINT:
      if (want == LJ_WASM_TYPE_F64) {
	lj_wasm_putu8(body, LJ_WASM_OP_F64_CONST);
	wasm_putf64(body, (lua_Number)ir->i);
      } else if (want == LJ_WASM_TYPE_I64) {
	lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
	lj_wasm_puti64v(body, (int64_t)ir->i);
      } else {
	lj_wasm_putu8(body, LJ_WASM_OP_I32_CONST);
	lj_wasm_puti32v(body, ir->i);
      }
      return 1;
    case IR_KNUM:
      if (want != LJ_WASM_TYPE_F64)
	return 0;
      lj_wasm_putu8(body, LJ_WASM_OP_F64_CONST);
      wasm_putf64(body, numV(ir_knum(ir)));
      return 1;
    case IR_KINT64:
      if (want != LJ_WASM_TYPE_I64)
	return 0;
      lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
      lj_wasm_puti64v(body, (int64_t)ir_kint64(ir)->u64);
      return 1;
    case IR_KNULL:
      if (want != LJ_WASM_TYPE_I64 && want != LJ_WASM_TYPE_I32)
	return 0;
      lj_wasm_putu8(body, want == LJ_WASM_TYPE_I64 ?
		    LJ_WASM_OP_I64_CONST : LJ_WASM_OP_I32_CONST);
      if (want == LJ_WASM_TYPE_I64)
	lj_wasm_puti64v(body, 0);
      else
	lj_wasm_puti32v(body, 0);
      return 1;
    default:
      return 0;
    }
  }
}

static int wasm_trace_supported(const GCtrace *T)
{
  IRRef ref;
  for (ref = REF_FIRST; ref < T->nins; ref++) {
    IRIns *ir = &T->ir[ref];
    switch ((IROp)ir->o) {
    case IR_NOP:
    case IR_RENAME:
    case IR_LOOP:
      break;
    case IR_SLOAD:
      if (!(irt_isint(ir->t) || irt_isnum(ir->t) || irt_istab(ir->t)))
	return 0;
      if ((ir->op2 & (IRSLOAD_PARENT|IRSLOAD_FRAME|IRSLOAD_KEYINDEX)))
	return 0;
      if ((ir->op2 & IRSLOAD_CONVERT) && !irt_isint(ir->t) &&
	  !irt_isnum(ir->t))
	return 0;
      if ((ir->op2 & IRSLOAD_TYPECHECK) && !irt_isguard(ir->t))
	return 0;
      if ((ir->op2 & (IRSLOAD_CONVERT|IRSLOAD_TYPECHECK)) ==
	  (IRSLOAD_CONVERT|IRSLOAD_TYPECHECK))
	return 0;
      break;
    case IR_FLOAD:
      if (!wasm_ref_can_type(T, ir->op1, LJ_WASM_TYPE_I64))
	return 0;
      if (ir->op2 == IRFL_TAB_ASIZE) {
	if (!irt_isint(ir->t))
	  return 0;
      } else if (ir->op2 == IRFL_TAB_ARRAY || ir->op2 == IRFL_TAB_META) {
	if (wasm_ir_valtype(ir) != LJ_WASM_TYPE_I64)
	  return 0;
      } else {
	return 0;
      }
      break;
    case IR_ABC:
      if (!wasm_ref_can_type(T, ir->op1, LJ_WASM_TYPE_I32) ||
	  !wasm_ref_can_type(T, ir->op2, LJ_WASM_TYPE_I32))
	return 0;
      break;
    case IR_AREF:
      if (wasm_ir_valtype(ir) != LJ_WASM_TYPE_I64 ||
	  !wasm_ref_can_type(T, ir->op1, LJ_WASM_TYPE_I64) ||
	  !wasm_ref_can_type(T, ir->op2, LJ_WASM_TYPE_I32))
	return 0;
      break;
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
      if (!(irt_isint(ir->t) || irt_isnum(ir->t)))
	return 0;
      if (!wasm_ref_can_type(T, ir->op1, wasm_ir_valtype(ir)) ||
	  !wasm_ref_can_type(T, ir->op2, wasm_ir_valtype(ir)))
	return 0;
      break;
    case IR_CONV:
      if (ir->op2 != IRCONV_NUM_INT || !irt_isnum(ir->t) ||
	  !wasm_ref_can_type(T, ir->op1, LJ_WASM_TYPE_I32))
	return 0;
      break;
    case IR_LE: {
      uint8_t type = wasm_ref_fixed_type(T, ir->op1);
      if (!type) type = wasm_ref_fixed_type(T, ir->op2);
      if (!type) type = wasm_ir_valtype(ir);
      if (type != LJ_WASM_TYPE_I32 && type != LJ_WASM_TYPE_F64)
	return 0;
      if (!wasm_ref_can_type(T, ir->op1, type) ||
	  !wasm_ref_can_type(T, ir->op2, type))
	return 0;
      break;
      }
    case IR_PHI:
      if (!(irt_isint(ir->t) || irt_isnum(ir->t)))
	return 0;
      if (!wasm_ref_can_type(T, ir->op1, wasm_ir_valtype(ir)))
	return 0;
      break;
    default:
      return 0;
    }
  }
  return T->nins > REF_FIRST;
}

static int wasm_emit_arith(WasmTraceCtx *ctx, IRRef ref, IRIns *ir)
{
  uint8_t type = wasm_ir_valtype(ir);
  uint8_t op;
  if (!wasm_emit_ref(ctx, ir->op1, type) ||
      !wasm_emit_ref(ctx, ir->op2, type))
    return 0;
  if (type == LJ_WASM_TYPE_F64) {
    op = ir->o == IR_ADD ? LJ_WASM_OP_F64_ADD :
	 ir->o == IR_SUB ? LJ_WASM_OP_F64_SUB : LJ_WASM_OP_F64_MUL;
  } else {
    op = ir->o == IR_ADD ? LJ_WASM_OP_I32_ADD :
	 ir->o == IR_SUB ? LJ_WASM_OP_I32_SUB : LJ_WASM_OP_I32_MUL;
  }
  lj_wasm_putu8(ctx->body, op);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_LOCAL_SET);
  lj_wasm_putu32v(ctx->body, wasm_ir_local(ref));
  return 1;
}

static int wasm_emit_conv(WasmTraceCtx *ctx, IRRef ref, IRIns *ir)
{
  if (ir->op2 != IRCONV_NUM_INT || !irt_isnum(ir->t) ||
      !wasm_emit_ref(ctx, ir->op1, LJ_WASM_TYPE_I32))
    return 0;
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_F64_CONVERT_I32_S);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_LOCAL_SET);
  lj_wasm_putu32v(ctx->body, wasm_ir_local(ref));
  return 1;
}

static void wasm_emit_guard_exit_zero(SBuf *body)
{
  lj_wasm_putu8(body, LJ_WASM_OP_I32_CONST);
  lj_wasm_puti32v(body, 0);
  lj_wasm_putu8(body, LJ_WASM_OP_RETURN);
}

static int wasm_emit_abc_guard(WasmTraceCtx *ctx, IRIns *ir)
{
  if (!wasm_emit_ref(ctx, ir->op2, LJ_WASM_TYPE_I32) ||
      !wasm_emit_ref(ctx, ir->op1, LJ_WASM_TYPE_I32))
    return 0;
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_I32_GE_U);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_IF);
  lj_wasm_putu8(ctx->body, LJ_WASM_BLOCKTYPE_EMPTY);
  wasm_emit_guard_exit_zero(ctx->body);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_END);
  return 1;
}

static int wasm_emit_aref(WasmTraceCtx *ctx, IRRef ref, IRIns *ir)
{
  if (!wasm_emit_ref(ctx, ir->op1, LJ_WASM_TYPE_I64) ||
      !wasm_emit_ref(ctx, ir->op2, LJ_WASM_TYPE_I32))
    return 0;
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_I64_EXTEND_I32_U);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_I64_CONST);
  lj_wasm_puti64v(ctx->body, 3);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_I64_SHL);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_I64_ADD);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_LOCAL_SET);
  lj_wasm_putu32v(ctx->body, wasm_ir_local(ref));
  return 1;
}

static int wasm_emit_le_guard(WasmTraceCtx *ctx, IRIns *ir)
{
  uint8_t type = wasm_ref_fixed_type(ctx->T, ir->op1);
  if (!type) type = wasm_ref_fixed_type(ctx->T, ir->op2);
  if (!type) type = wasm_ir_valtype(ir);
  if (!wasm_emit_ref(ctx, ir->op1, type) ||
      !wasm_emit_ref(ctx, ir->op2, type))
    return 0;
  lj_wasm_putu8(ctx->body, type == LJ_WASM_TYPE_F64 ?
		LJ_WASM_OP_F64_LE : LJ_WASM_OP_I32_LE_S);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_I32_EQZ);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_IF);
  lj_wasm_putu8(ctx->body, LJ_WASM_BLOCKTYPE_EMPTY);
  wasm_emit_guard_exit_zero(ctx->body);
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_END);
  return 1;
}

static int wasm_emit_phi(WasmTraceCtx *ctx, IRRef ref, IRIns *ir)
{
  uint8_t type = wasm_ir_valtype(ir);
  if (!wasm_emit_ref(ctx, ir->op1, type))
    return 0;
  lj_wasm_putu8(ctx->body, LJ_WASM_OP_LOCAL_SET);
  lj_wasm_putu32v(ctx->body, wasm_ir_local(ref));
  return 1;
}

static int32_t wasm_sload_ofs(IRIns *ir)
{
  return 8 * ((int32_t)ir->op1 - 1 - LJ_FR2);
}

static void wasm_emit_addr_const_add(SBuf *body, int32_t ofs)
{
  lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_GET);
  lj_wasm_putu32v(body, 1);  /* Entry parameter: base. */
  if (ofs != 0) {
    lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
    lj_wasm_puti64v(body, ofs);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_ADD);
  }
}

static int wasm_emit_ref_const_add(WasmTraceCtx *ctx, IRRef ref, int32_t ofs)
{
  SBuf *body = ctx->body;
  if (!wasm_emit_ref(ctx, ref, LJ_WASM_TYPE_I64))
    return 0;
  if (ofs != 0) {
    lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
    lj_wasm_puti64v(body, ofs);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_ADD);
  }
  return 1;
}

static int wasm_emit_sload(WasmTraceCtx *ctx, IRRef ref, IRIns *ir)
{
  uint8_t type = wasm_ir_valtype(ir);
  SBuf *body = ctx->body;
  wasm_emit_addr_const_add(body, wasm_sload_ofs(ir));
  if (irt_istab(ir->t)) {
    lj_wasm_putu8(body, LJ_WASM_OP_I64_LOAD);
    lj_wasm_putmemarg(body, 3, 0);
    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
    lj_wasm_putu32v(body, ctx->tmp64);

    if ((ir->op2 & IRSLOAD_TYPECHECK)) {
      lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_GET);
      lj_wasm_putu32v(body, ctx->tmp64);
      lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
      lj_wasm_puti64v(body, 47);
      lj_wasm_putu8(body, LJ_WASM_OP_I64_SHR_U);
      lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
      lj_wasm_puti64v(body, (int64_t)wasm_gc64_tag(LJ_TTAB));
      lj_wasm_putu8(body, LJ_WASM_OP_I64_EQ);
      lj_wasm_putu8(body, LJ_WASM_OP_I32_EQZ);
      lj_wasm_putu8(body, LJ_WASM_OP_IF);
      lj_wasm_putu8(body, LJ_WASM_BLOCKTYPE_EMPTY);
      wasm_emit_guard_exit_zero(body);
      lj_wasm_putu8(body, LJ_WASM_OP_END);
    }

    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_GET);
    lj_wasm_putu32v(body, ctx->tmp64);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
    lj_wasm_puti64v(body, LJ_GCVMASK);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_AND);
    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
    lj_wasm_putu32v(body, wasm_ir_local(ref));
    return 1;
  }
  if ((ir->op2 & IRSLOAD_CONVERT)) {
    if (type == LJ_WASM_TYPE_F64) {
      lj_wasm_putu8(body, LJ_WASM_OP_I32_LOAD);
      lj_wasm_putmemarg(body, 2, 0);
      lj_wasm_putu8(body, LJ_WASM_OP_F64_CONVERT_I32_S);
    } else if (type == LJ_WASM_TYPE_I32) {
      lj_wasm_putu8(body, LJ_WASM_OP_F64_LOAD);
      lj_wasm_putmemarg(body, 3, 0);
      lj_wasm_putu8(body, LJ_WASM_OP_I32_TRUNC_F64_S);
    } else {
      return 0;
    }
    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
    lj_wasm_putu32v(body, wasm_ir_local(ref));
    return 1;
  }
  if ((ir->op2 & IRSLOAD_TYPECHECK)) {
    lj_wasm_putu8(body, LJ_WASM_OP_I64_LOAD);
    lj_wasm_putmemarg(body, 3, 0);
    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
    lj_wasm_putu32v(body, ctx->tmp64);

    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_GET);
    lj_wasm_putu32v(body, ctx->tmp64);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
    lj_wasm_puti64v(body, 32);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_SHR_U);
    lj_wasm_putu8(body, LJ_WASM_OP_I64_CONST);
    lj_wasm_puti64v(body, (int64_t)(uint32_t)(LJ_TISNUM << 15));
    lj_wasm_putu8(body, type == LJ_WASM_TYPE_F64 ?
		  LJ_WASM_OP_I64_LT_U : LJ_WASM_OP_I64_EQ);
    lj_wasm_putu8(body, LJ_WASM_OP_I32_EQZ);
    lj_wasm_putu8(body, LJ_WASM_OP_IF);
    lj_wasm_putu8(body, LJ_WASM_BLOCKTYPE_EMPTY);
    wasm_emit_guard_exit_zero(body);
    lj_wasm_putu8(body, LJ_WASM_OP_END);

    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_GET);
    lj_wasm_putu32v(body, ctx->tmp64);
    lj_wasm_putu8(body, type == LJ_WASM_TYPE_F64 ?
		  LJ_WASM_OP_F64_REINTERPRET_I64 : LJ_WASM_OP_I32_WRAP_I64);
    lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
    lj_wasm_putu32v(body, wasm_ir_local(ref));
    return 1;
  }
  if (type == LJ_WASM_TYPE_F64) {
    lj_wasm_putu8(body, LJ_WASM_OP_F64_LOAD);
    lj_wasm_putmemarg(body, 3, 0);
  } else if (type == LJ_WASM_TYPE_I32) {
    lj_wasm_putu8(body, LJ_WASM_OP_I32_LOAD);
    lj_wasm_putmemarg(body, 2, 0);
  } else {
    return 0;
  }
  lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
  lj_wasm_putu32v(body, wasm_ir_local(ref));
  return 1;
}

static int wasm_emit_fload(WasmTraceCtx *ctx, IRRef ref, IRIns *ir)
{
  SBuf *body = ctx->body;
  int32_t ofs;
  uint8_t op, align;
  switch (ir->op2) {
  case IRFL_TAB_ASIZE:
    ofs = (int32_t)offsetof(GCtab, asize);
    op = LJ_WASM_OP_I32_LOAD;
    align = 2;
    break;
  case IRFL_TAB_ARRAY:
    ofs = (int32_t)offsetof(GCtab, array);
    op = LJ_WASM_OP_I64_LOAD;
    align = 3;
    break;
  case IRFL_TAB_META:
    ofs = (int32_t)offsetof(GCtab, metatable);
    op = LJ_WASM_OP_I64_LOAD;
    align = 3;
    break;
  default:
    return 0;
  }
  if (!wasm_emit_ref_const_add(ctx, ir->op1, ofs))
    return 0;
  lj_wasm_putu8(body, op);
  lj_wasm_putmemarg(body, align, 0);
  lj_wasm_putu8(body, LJ_WASM_OP_LOCAL_SET);
  lj_wasm_putu32v(body, wasm_ir_local(ref));
  return 1;
}

static int wasm_emit_trace_body(lua_State *L, const GCtrace *T, SBuf *body)
{
  WasmTraceCtx ctx;
  IRRef ref, nlocals = T->nins > REF_FIRST ? T->nins - REF_FIRST : 0;
  UNUSED(L);
  ctx.T = T;
  ctx.body = body;
  ctx.tmp64 = wasm_tmp64_local(T);

  lj_wasm_putu32v(body, (uint32_t)nlocals + 1);
  for (ref = REF_FIRST; ref < T->nins; ref++) {
    lj_wasm_putu32v(body, 1);
    lj_wasm_putu8(body, wasm_ir_valtype(&T->ir[ref]));
  }
  lj_wasm_putu32v(body, 1);
  lj_wasm_putu8(body, LJ_WASM_TYPE_I64);

  for (ref = REF_FIRST; ref < T->nins; ref++) {
    IRIns *ir = &T->ir[ref];
    switch ((IROp)ir->o) {
    case IR_NOP:
    case IR_RENAME:
      break;
    case IR_LOOP:
      lj_wasm_putu8(body, LJ_WASM_OP_NOP);
      break;
    case IR_SLOAD: {
      if (!wasm_emit_sload(&ctx, ref, ir))
	return 0;
      break;
      }
    case IR_FLOAD:
      if (!wasm_emit_fload(&ctx, ref, ir))
	return 0;
      break;
    case IR_ABC:
      if (!wasm_emit_abc_guard(&ctx, ir))
	return 0;
      break;
    case IR_AREF:
      if (!wasm_emit_aref(&ctx, ref, ir))
	return 0;
      break;
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
      if (!wasm_emit_arith(&ctx, ref, ir))
	return 0;
      break;
    case IR_CONV:
      if (!wasm_emit_conv(&ctx, ref, ir))
	return 0;
      break;
    case IR_LE:
      if (!wasm_emit_le_guard(&ctx, ir))
	return 0;
      break;
    case IR_PHI:
      if (!wasm_emit_phi(&ctx, ref, ir))
	return 0;
      break;
    default:
      return 0;
    }
  }

  lj_wasm_putu8(body, LJ_WASM_OP_I32_CONST);
  lj_wasm_puti32v(body, LJ_WASM_JIT_STATUS_NYI);
  lj_wasm_putu8(body, LJ_WASM_OP_END);
  return 1;
}

int lj_wasm_jit_build_trace(lua_State *L, const GCtrace *T, SBuf *module)
{
  SBuf body;
  int lowered;
  if (!wasm_trace_supported(T)) {
    lj_wasm_jit_build_nyi(L, module);
    return 0;
  }

  lj_buf_init(L, &body);
  lowered = wasm_emit_trace_body(L, T, &body);
  if (lowered)
    wasm_build_module(L, module, &body, 1);
  wasm_sbuf_free(L, &body);
  if (!lowered)
    lj_wasm_jit_build_nyi(L, module);
  return lowered;
}

#endif

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
  module.memory_min = 0;
  module.memory_max = 0;
  module.memory_flags = 0;
  module.reserved = 0;

  status = lj_wasm_host_jit_compile(&module, handle);

  if (sb.b)
    lj_buf_free(G(L), &sb);
  return status;
}

#if LJ_HASJIT
int lj_wasm_jit_compile_trace(lua_State *L, const GCtrace *T,
			      LJWasmHostHandle *handle)
{
  LJWasmJITModule module;
  SBuf sb;
  int lowered, status;

  lj_buf_init(L, &sb);
  lowered = lj_wasm_jit_build_trace(L, T, &sb);

  module.bytes = (const uint8_t *)sb.b;
  module.size = sbuflen(&sb);
  module.trace = T->traceno;
  module.entry = 0;
  module.exit = 0;
  module.flags = lowered ? (LJ_WASM_JIT_F_IR_LOWERED |
			    LJ_WASM_JIT_F_IMPORT_ENV_MEMORY) : 0;
  module.memory_min = lowered ? 1 : 0;
  module.memory_max = 0;
  module.memory_flags = lowered ? LJ_WASM_JIT_MEMORY_F_64 : 0;
  module.reserved = 0;

  status = lj_wasm_host_jit_compile(&module, handle);

  if (sb.b)
    lj_buf_free(G(L), &sb);
  return status;
}
#endif
#endif
