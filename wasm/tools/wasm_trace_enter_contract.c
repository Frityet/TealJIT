/*
** Check the WASM trace-entry helper contract in a native tools build.
*/

#define LUA_CORE

#include "lua.h"
#include "lj_obj.h"
#include "lj_vm_wasm.h"

int main(void)
{
#if LJ_HASJIT
  return lj_vm_wasm_trace_enter(NULL, NULL, 1) == -2 ? 0 : 1;
#else
  return 0;
#endif
}
