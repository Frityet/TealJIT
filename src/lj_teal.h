/*
** Native Teal front-end.
** Copyright (C) 2026 TealJIT contributors.
*/

#ifndef _LJ_TEAL_H
#define _LJ_TEAL_H

#include "lj_obj.h"
#include "lj_lex.h"

LJ_FUNC int lj_teal_enabled(LexState *ls);
LJ_FUNC GCproto *lj_teal_parse(LexState *ls);
LJ_FUNC void lj_teal_libinit(lua_State *L);
LUA_API int lj_teal_option(const char *opt);

#endif
