/*
** Emit the IR shape for a simple Lua table array-read loop as a WASM64 trace
** module. This is a developer tool, not part of the LuaJIT runtime.
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

static void setkint(GCtrace *T, IRRef ref, int32_t k)
{
  IRIns *ir = &T->ir[ref];
  ir->ot = IRT(IR_KINT, IRT_INT);
  ir->i = k;
  ir->prev = 0;
}

int main(void)
{
  enum { NIR = 25, K100 = REF_BASE-6, K3 = REF_BASE-5, K1 = REF_BASE-4 };
  lua_State *L = luaL_newstate();
  IRIns *ir = (IRIns *)calloc(REF_FIRST + NIR, sizeof(IRIns));
  GCtrace T;
  SBuf sb;
  MSize len;
  int lowered, ok;

  if (!L || !ir)
    return 1;

  memset(&T, 0, sizeof(T));
  T.traceno = 4;
  T.nk = K100;
  T.nins = REF_FIRST + NIR;
  T.ir = ir;

  setkint(&T, K100, 100);
  setkint(&T, K3, 3);
  setkint(&T, K1, 1);

  setir(&T, REF_FIRST+0, IR_SLOAD, IRT_INT, 4,
	IRSLOAD_CONVERT|IRSLOAD_INHERIT);
  setir(&T, REF_FIRST+1, IR_MOD, IRT_INT, REF_FIRST+0, K3);
  setir(&T, REF_FIRST+2, IR_NOP, IRT_NIL, 0, 0);
  setguard(&T, REF_FIRST+3, IR_SLOAD, IRT_TAB, 2, IRSLOAD_TYPECHECK);
  setir(&T, REF_FIRST+4, IR_ADD, IRT_INT, REF_FIRST+1, K1);
  setir(&T, REF_FIRST+5, IR_FLOAD, IRT_INT, REF_FIRST+3, IRFL_TAB_ASIZE);
  setguard(&T, REF_FIRST+6, IR_ABC, IRT_INT, REF_FIRST+5, REF_FIRST+4);
  setir(&T, REF_FIRST+7, IR_FLOAD, IRT_P64, REF_FIRST+3, IRFL_TAB_ARRAY);
  setir(&T, REF_FIRST+8, IR_AREF, IRT_P64, REF_FIRST+7, REF_FIRST+4);
  setguard(&T, REF_FIRST+9, IR_ALOAD, IRT_NUM, REF_FIRST+8, 0);
  setguard(&T, REF_FIRST+10, IR_SLOAD, IRT_NUM, 3, IRSLOAD_TYPECHECK);
  setir(&T, REF_FIRST+11, IR_ADD, IRT_NUM, REF_FIRST+10, REF_FIRST+9);
  setir(&T, REF_FIRST+12, IR_ADD, IRT_INT, REF_FIRST+0, K1);
  setguard(&T, REF_FIRST+13, IR_LE, IRT_INT, REF_FIRST+12, K100);
  setir(&T, REF_FIRST+14, IR_LOOP, IRT_NIL, 0, 0);
  setir(&T, REF_FIRST+15, IR_MOD, IRT_INT, REF_FIRST+12, K3);
  setir(&T, REF_FIRST+16, IR_ADD, IRT_INT, REF_FIRST+15, K1);
  setguard(&T, REF_FIRST+17, IR_ABC, IRT_INT, REF_FIRST+5, REF_FIRST+16);
  setir(&T, REF_FIRST+18, IR_AREF, IRT_P64, REF_FIRST+7, REF_FIRST+16);
  setguard(&T, REF_FIRST+19, IR_ALOAD, IRT_NUM, REF_FIRST+18, 0);
  setir(&T, REF_FIRST+20, IR_ADD, IRT_NUM, REF_FIRST+19, REF_FIRST+11);
  setir(&T, REF_FIRST+21, IR_ADD, IRT_INT, REF_FIRST+12, K1);
  setguard(&T, REF_FIRST+22, IR_LE, IRT_INT, REF_FIRST+21, K100);
  setir(&T, REF_FIRST+23, IR_PHI, IRT_INT, REF_FIRST+12, REF_FIRST+21);
  setir(&T, REF_FIRST+24, IR_PHI, IRT_NUM, REF_FIRST+11, REF_FIRST+20);

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
