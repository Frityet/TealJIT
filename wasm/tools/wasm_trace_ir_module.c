/*
** Emit a small IR-lowered LuaJIT trace module for WASM64 JIT bring-up.
** This is a developer tool, not part of the LuaJIT runtime.
*/

#define LUA_CORE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lj_obj.h"
#include "lj_ir.h"
#include "lj_jit.h"
#include "lj_buf.h"
#include "lj_wasm_jit.h"

static void setir(GCtrace *T, IRRef ref, IROp op, IRType type,
		  IRRef op1, IRRef op2)
{
  IRIns *ir = &T->ir[ref];
  ir->ot = IRT(op, type);
  ir->op1 = (IRRef1)op1;
  ir->op2 = (IRRef1)op2;
  ir->prev = 0;
}

static void setguard(GCtrace *T, IRRef ref, IROp op, IRType type,
		     IRRef op1, IRRef op2)
{
  IRIns *ir = &T->ir[ref];
  ir->ot = IRTG(op, type);
  ir->op1 = (IRRef1)op1;
  ir->op2 = (IRRef1)op2;
  ir->prev = 0;
}

static void setknull(GCtrace *T, IRRef ref)
{
  IRIns *ir = &T->ir[ref];
  ir->ot = IRT(IR_KNULL, IRT_P64);
  ir->op1 = 0;
  ir->op2 = 0;
  ir->prev = 0;
}

int main(void)
{
  enum { NIR = 23, KNULL = REF_BASE-4 };
  lua_State *L = luaL_newstate();
  IRIns *ir = (IRIns *)calloc(REF_FIRST + NIR, sizeof(IRIns));
  GCtrace T;
  SBuf sb;
  MSize len;
  int lowered, ok;

  if (!L || !ir)
    return 1;

  memset(&T, 0, sizeof(T));
  T.traceno = 1;
  T.nk = KNULL;
  T.nins = REF_FIRST + NIR;
  T.ir = ir;

  setknull(&T, KNULL);

  setir(&T, REF_FIRST+0, IR_SLOAD, IRT_INT, 3, IRSLOAD_READONLY);
  setir(&T, REF_FIRST+1, IR_ADD, IRT_INT, REF_FIRST+0, REF_FIRST+0);
  setguard(&T, REF_FIRST+2, IR_LE, IRT_INT, REF_FIRST+1, REF_FIRST+1);
  setguard(&T, REF_FIRST+3, IR_SLOAD, IRT_INT, 4, IRSLOAD_TYPECHECK);
  setir(&T, REF_FIRST+4, IR_CONV, IRT_NUM, REF_FIRST+0, IRCONV_NUM_INT);
  setir(&T, REF_FIRST+5, IR_SLOAD, IRT_NUM, 2, IRSLOAD_READONLY);
  setir(&T, REF_FIRST+6, IR_ADD, IRT_NUM, REF_FIRST+5, REF_FIRST+5);
  setguard(&T, REF_FIRST+7, IR_LE, IRT_NUM, REF_FIRST+6, REF_FIRST+6);
  setguard(&T, REF_FIRST+8, IR_SLOAD, IRT_NUM, 5, IRSLOAD_TYPECHECK);
  setir(&T, REF_FIRST+9, IR_SLOAD, IRT_INT, 6, IRSLOAD_CONVERT);
  setir(&T, REF_FIRST+10, IR_SLOAD, IRT_NUM, 7, IRSLOAD_CONVERT);
  setguard(&T, REF_FIRST+11, IR_SLOAD, IRT_TAB, 8, IRSLOAD_TYPECHECK);
  setir(&T, REF_FIRST+12, IR_FLOAD, IRT_INT, REF_FIRST+11, IRFL_TAB_ASIZE);
  setir(&T, REF_FIRST+13, IR_FLOAD, IRT_P64, REF_FIRST+11, IRFL_TAB_ARRAY);
  setir(&T, REF_FIRST+14, IR_FLOAD, IRT_TAB, REF_FIRST+11, IRFL_TAB_META);
  setguard(&T, REF_FIRST+15, IR_ABC, IRT_INT, REF_FIRST+12, REF_FIRST+0);
  setir(&T, REF_FIRST+16, IR_AREF, IRT_P64, REF_FIRST+13, REF_FIRST+0);
  setguard(&T, REF_FIRST+17, IR_ALOAD, IRT_NUM, REF_FIRST+16, 0);
  setir(&T, REF_FIRST+18, IR_ASTORE, IRT_NUM, REF_FIRST+16, REF_FIRST+4);
  setguard(&T, REF_FIRST+19, IR_EQ, IRT_P64, REF_FIRST+14, KNULL);
  setir(&T, REF_FIRST+20, IR_LOOP, IRT_NIL, 0, 0);
  setir(&T, REF_FIRST+21, IR_PHI, IRT_INT, REF_FIRST+0, REF_FIRST+1);
  setir(&T, REF_FIRST+22, IR_PHI, IRT_NUM, REF_FIRST+5, REF_FIRST+6);

  lj_buf_init(L, &sb);
  lowered = lj_wasm_jit_build_trace(L, &T, &sb);
  len = sbuflen(&sb);
  ok = lowered && fwrite(sb.b, 1, len, stdout) == len;

  if (sb.b)
    lj_buf_free(G(L), &sb);
  free(ir);
  lua_close(L);
  return ok ? 0 : 1;
}
