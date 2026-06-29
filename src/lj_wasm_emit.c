/*
** WebAssembly binary emitter.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_wasm_emit_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_buf.h"
#include "lj_wasm_emit.h"

/* -- Low-level writes ---------------------------------------------------- */

void lj_wasm_module_begin(SBuf *sb)
{
  static const uint8_t hdr[] = {
    0x00, 0x61, 0x73, 0x6d,  /* magic: "\0asm" */
    0x01, 0x00, 0x00, 0x00   /* version 1 */
  };
  lj_buf_putmem(sb, hdr, (MSize)sizeof(hdr));
}

void lj_wasm_putu8(SBuf *sb, uint8_t x)
{
  lj_buf_putb(sb, x);
}

void lj_wasm_putu32v(SBuf *sb, uint32_t x)
{
  do {
    uint8_t b = (uint8_t)(x & 0x7f);
    x >>= 7;
    if (x) b |= 0x80;
    lj_buf_putb(sb, b);
  } while (x);
}

void lj_wasm_putu64v(SBuf *sb, uint64_t x)
{
  do {
    uint8_t b = (uint8_t)(x & 0x7f);
    x >>= 7;
    if (x) b |= 0x80;
    lj_buf_putb(sb, b);
  } while (x);
}

void lj_wasm_puti32v(SBuf *sb, int32_t x)
{
  int more;
  do {
    uint8_t b = (uint8_t)(x & 0x7f);
    x >>= 7;
    more = !(((x == 0) && !(b & 0x40)) || ((x == -1) && (b & 0x40)));
    if (more) b |= 0x80;
    lj_buf_putb(sb, b);
  } while (more);
}

void lj_wasm_puti64v(SBuf *sb, int64_t x)
{
  int more;
  do {
    uint8_t b = (uint8_t)(x & 0x7f);
    x >>= 7;
    more = !(((x == 0) && !(b & 0x40)) || ((x == -1) && (b & 0x40)));
    if (more) b |= 0x80;
    lj_buf_putb(sb, b);
  } while (more);
}

void lj_wasm_putbytes(SBuf *sb, const void *p, MSize len)
{
  if (len)
    lj_buf_putmem(sb, p, len);
}

void lj_wasm_putname(SBuf *sb, const char *name, MSize len)
{
  lj_wasm_putu32v(sb, len);
  lj_wasm_putbytes(sb, name, len);
}

/* -- Structured writes --------------------------------------------------- */

void lj_wasm_putsection(SBuf *sb, uint8_t id, const SBuf *payload)
{
  MSize len = sbuflen(payload);
  lj_wasm_putu8(sb, id);
  lj_wasm_putu32v(sb, len);
  lj_wasm_putbytes(sb, payload->b, len);
}

void lj_wasm_putfunctype(SBuf *sb, const uint8_t *params, MSize nparams,
			 const uint8_t *results, MSize nresults)
{
  MSize i;
  lj_wasm_putu8(sb, 0x60);  /* func */
  lj_wasm_putu32v(sb, nparams);
  for (i = 0; i < nparams; i++)
    lj_wasm_putu8(sb, params[i]);
  lj_wasm_putu32v(sb, nresults);
  for (i = 0; i < nresults; i++)
    lj_wasm_putu8(sb, results[i]);
}

void lj_wasm_putimport_func(SBuf *sb, const char *module, MSize module_len,
			    const char *name, MSize name_len, uint32_t typeidx)
{
  lj_wasm_putname(sb, module, module_len);
  lj_wasm_putname(sb, name, name_len);
  lj_wasm_putu8(sb, LJ_WASM_EXT_FUNC);
  lj_wasm_putu32v(sb, typeidx);
}

void lj_wasm_putexport(SBuf *sb, const char *name, MSize name_len,
		       uint8_t kind, uint32_t idx)
{
  lj_wasm_putname(sb, name, name_len);
  lj_wasm_putu8(sb, kind);
  lj_wasm_putu32v(sb, idx);
}

void lj_wasm_putlimits(SBuf *sb, uint64_t min, uint64_t max, int hasmax,
		       int is64)
{
  uint8_t flags = (uint8_t)((hasmax ? 0x01 : 0x00) | (is64 ? 0x04 : 0x00));
  lj_wasm_putu8(sb, flags);
  if (is64) {
    lj_wasm_putu64v(sb, min);
    if (hasmax)
      lj_wasm_putu64v(sb, max);
  } else {
    lj_wasm_putu32v(sb, (uint32_t)min);
    if (hasmax)
      lj_wasm_putu32v(sb, (uint32_t)max);
  }
}

void lj_wasm_putfuncbody(SBuf *sb, const SBuf *body)
{
  MSize len = sbuflen(body);
  lj_wasm_putu32v(sb, len);
  lj_wasm_putbytes(sb, body->b, len);
}
