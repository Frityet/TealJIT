/*
** Definitions for WebAssembly targets.
** Copyright (C) 2005-2026 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_TARGET_WASM_H
#define _LJ_TARGET_WASM_H

/* -- Local IDs ----------------------------------------------------------- */

/*
** WebAssembly is a stack VM, but LuaJIT's register allocator needs a bounded
** set of target locations. These IDs model typed Wasm locals for the future
** IR-to-Wasm emitter and are intentionally not hardware registers.
*/
#define GPRDEF(_) \
  _(I0) _(I1) _(I2) _(I3) _(I4) _(I5) _(I6) _(I7) \
  _(I8) _(I9) _(I10) _(I11) _(I12) _(I13) _(I14) _(I15)
#define FPRDEF(_) \
  _(F0) _(F1) _(F2) _(F3) _(F4) _(F5) _(F6) _(F7) \
  _(F8) _(F9) _(F10) _(F11) _(F12) _(F13) _(F14) _(F15)
#define VRIDDEF(_)

#define RIDENUM(name)	RID_##name,

enum {
  GPRDEF(RIDENUM)		/* Integer/reference Wasm locals. */
  FPRDEF(RIDENUM)		/* Floating-point Wasm locals. */
  RID_MAX,
  RID_TMP = RID_I15,

  /* Calling conventions for host-assisted trace exits and FFI shims. */
  RID_RET = RID_I0,
  RID_RETLO = RID_I0,
  RID_RETHI = RID_I1,
  RID_FPRET = RID_F0,

  /* These definitions must match the future vm_wasm64 backend. */
  RID_BASE = RID_I8,		/* Interpreter BASE. */
  RID_LPC = RID_I9,		/* Interpreter PC. */
  RID_GL = RID_I10,		/* Interpreter GL. */
  RID_LREG = RID_I11,		/* Interpreter L. */

  /* Register ranges [min, max) and number of locals. */
  RID_MIN_GPR = RID_I0,
  RID_MAX_GPR = RID_I15+1,
  RID_MIN_FPR = RID_MAX_GPR,
  RID_MAX_FPR = RID_F15+1,
  RID_NUM_GPR = RID_MAX_GPR - RID_MIN_GPR,
  RID_NUM_FPR = RID_MAX_FPR - RID_MIN_FPR
};

#define RID_NUM_KREF		RID_NUM_GPR
#define RID_MIN_KREF		RID_I0

/* -- Register sets ------------------------------------------------------- */

#define RSET_FIXED \
  (RID2RSET(RID_BASE)|RID2RSET(RID_LPC)|RID2RSET(RID_GL)|RID2RSET(RID_LREG))
#define RSET_GPR	(RSET_RANGE(RID_MIN_GPR, RID_MAX_GPR) & ~RSET_FIXED)
#define RSET_FPR	RSET_RANGE(RID_MIN_FPR, RID_MAX_FPR)
#define RSET_ALL	(RSET_GPR|RSET_FPR)
#define RSET_INIT	RSET_ALL

#define RSET_SCRATCH_GPR	RSET_RANGE(RID_I0, RID_I7+1)
#define RSET_SCRATCH_FPR	RSET_RANGE(RID_F0, RID_F7+1)
#define RSET_SCRATCH		(RSET_SCRATCH_GPR|RSET_SCRATCH_FPR)
#define REGARG_FIRSTGPR		RID_I0
#define REGARG_LASTGPR		RID_I7
#define REGARG_NUMGPR		8
#define REGARG_FIRSTFPR		RID_F0
#define REGARG_LASTFPR		RID_F7
#define REGARG_NUMFPR		8

/* -- Spill slots --------------------------------------------------------- */

/*
** Spill slots are 64 bit wide for WASM64. The encoder can lower i32/f32
** values through narrower load/store opcodes when materializing Wasm locals.
*/
#define SPS_FIXED	4
#define SPS_FIRST	2

#define SPOFS_TMP	0

#define sps_scale(slot)		(8 * (int32_t)(slot))
#define sps_align(slot)		(((slot) - SPS_FIXED + 1) & ~1)

/* -- Exit state ---------------------------------------------------------- */

typedef struct {
  lua_Number fpr[RID_NUM_FPR];	/* Floating-point locals. */
  intptr_t gpr[RID_NUM_GPR];	/* Integer/reference locals. */
  int64_t spill[256];		/* Spill slots. */
} ExitState;

/* Highest exit + 1 indicates stack check. */
#define EXITSTATE_CHECKEXIT	1

#endif
