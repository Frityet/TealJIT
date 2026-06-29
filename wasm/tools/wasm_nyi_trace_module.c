/*
** Emit the placeholder LuaJIT trace module used by WASM64 JIT bring-up.
** This is a developer tool, not part of the LuaJIT runtime.
*/

#define LUA_CORE

#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"
#include "lj_obj.h"
#include "lj_buf.h"
#include "lj_wasm_jit.h"

int main(void)
{
  lua_State *L = luaL_newstate();
  SBuf sb;
  MSize len;
  int ok;

  if (!L)
    return 1;

  lj_buf_init(L, &sb);
  lj_wasm_jit_build_nyi(L, &sb);

  len = sbuflen(&sb);
  ok = fwrite(sb.b, 1, len, stdout) == len;

  if (sb.b)
    lj_buf_free(G(L), &sb);
  lua_close(L);
  return ok ? 0 : 1;
}
