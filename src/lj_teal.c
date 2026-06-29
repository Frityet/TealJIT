/*
** Native Teal mode glue.
** Copyright (C) 2026 TealJIT contributors.
*/

#define lj_teal_c
#define LUA_CORE

#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include "lj_obj.h"
#include "lj_lex.h"
#include "lj_parse.h"
#include "lj_teal.h"

#define TEAL_FREEZE_RECORD "__tealjit_freeze_record"

static int teal_default_strict = 1;

static int teal_record_newindex(lua_State *L)
{
  UNUSED(L);
  return luaL_error(L, "Teal: frozen record cannot add field");
}

static int teal_record_freeze(lua_State *L)
{
  lua_settop(L, 1);
  if (!lua_istable(L, 1))
    return 1;
  if (lua_getmetatable(L, 1)) {
    lua_getfield(L, -1, "__tealjit_frozen_record");
    if (lua_toboolean(L, -1)) {
      lua_settop(L, 1);
      return 1;
    }
    /* Preserve user metatables until native record metatable merging exists. */
    lua_settop(L, 1);
    return 1;
  }
  lua_newtable(L);
  lua_pushcfunction(L, teal_record_newindex);
  lua_setfield(L, -2, "__newindex");
  lua_pushstring(L, "frozen Teal record");
  lua_setfield(L, -2, "__metatable");
  lua_pushboolean(L, 1);
  lua_setfield(L, -2, "__tealjit_frozen_record");
  lua_setmetatable(L, 1);
  lua_settop(L, 1);
  return 1;
}

LJ_FUNC void lj_teal_libinit(lua_State *L)
{
  lua_pushcfunction(L, teal_record_freeze);
  lua_setglobal(L, TEAL_FREEZE_RECORD);
}

static int teal_suffix_tl(const char *s)
{
  size_t len = strlen(s);
  return len >= 3 && s[len-3] == '.' && s[len-2] == 't' && s[len-1] == 'l';
}

LJ_FUNC int lj_teal_enabled(LexState *ls)
{
  const char *chunk = ls->chunkarg;
  if (chunk == NULL) return 0;
  if (chunk[0] == '@') chunk++;
  return teal_suffix_tl(chunk);
}

LUA_API int lj_teal_option(const char *opt)
{
  if (opt == NULL) return 0;
  if (strcmp(opt, "strict=on") == 0) {
    teal_default_strict = 1;
    return 1;
  }
  if (strcmp(opt, "strict=off") == 0) {
    teal_default_strict = 0;
    return 1;
  }
  return 0;
}

LJ_FUNC GCproto *lj_teal_parse(LexState *ls)
{
  ls->teal = 1;
  ls->teal_strict = teal_default_strict;
  return lj_parse(ls);
}
