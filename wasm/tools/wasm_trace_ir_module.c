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

int main(void)
{
  enum { NIR = 9 };
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
  T.nk = REF_BASE;
  T.nins = REF_FIRST + NIR;
  T.ir = ir;

  setir(&T, REF_FIRST+0, IR_SLOAD, IRT_INT, 3, IRSLOAD_READONLY);
  setir(&T, REF_FIRST+1, IR_ADD, IRT_INT, REF_FIRST+0, REF_FIRST+0);
  setguard(&T, REF_FIRST+2, IR_LE, IRT_INT, REF_FIRST+1, REF_FIRST+1);
  setir(&T, REF_FIRST+3, IR_SLOAD, IRT_NUM, 2, IRSLOAD_READONLY);
  setir(&T, REF_FIRST+4, IR_ADD, IRT_NUM, REF_FIRST+3, REF_FIRST+3);
  setguard(&T, REF_FIRST+5, IR_LE, IRT_NUM, REF_FIRST+4, REF_FIRST+4);
  setir(&T, REF_FIRST+6, IR_LOOP, IRT_NIL, 0, 0);
  setir(&T, REF_FIRST+7, IR_PHI, IRT_INT, REF_FIRST+0, REF_FIRST+1);
  setir(&T, REF_FIRST+8, IR_PHI, IRT_NUM, REF_FIRST+3, REF_FIRST+4);

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
