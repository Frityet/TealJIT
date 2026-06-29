# 20260629T210456Z - WASM loop/PHI lowering notes

- Branch: `wasm`
- HEAD inspected: `8b4344efe6bde00b5a4459f93dfdee263abce553`
- Scope: investigation only. No source edits or commits.

## Question 1: LuaJIT LOOP/PHI conventions

LuaJIT's loop optimizer emits a trace as:

1. pre-roll instructions before `IR_LOOP`
2. copied variant loop-body instructions after `IR_LOOP`
3. `IR_PHI` records at the bottom of the trace

This is documented directly in `src/lj_opt_loop.c`: the comments say `IR_LOOP`
separates the pre-roll from the loop body, and that PHIs are emitted below the
loop body. The implementation does that at `loop_unroll()`:

- `src/lj_opt_loop.c:43-85`: describes the pre-roll/body split and bottom PHIs.
- `src/lj_opt_loop.c:285-286`: emits `IR_LOOP`.
- `src/lj_opt_loop.c:310-380`: copy-substitutes the original instructions to
  produce the loop body.
- `src/lj_opt_loop.c:189-197`: emits each `IR_PHI(lref, rref)` after the body.

For a simple numeric `for`, the real x64 dump shape is:

```text
0001   int SLOAD #3 CI
0002 > num SLOAD #2 T
0003   num CONV  0001 num.int
0004 + num ADD   0003 0002
0005 + int ADD   0001 +1
0006 > int LE    0005 +3
0007 ------------ LOOP ------------
0008   num CONV  0005 num.int
0009 + num ADD   0008 0004
0010 + int ADD   0005 +1
0011 > int LE    0010 +3
0012   int PHI   0005 0010
0013   num PHI   0004 0009
---- TRACE 1 stop -> loop
```

The instructions after `IR_LOOP` refer to the pre-PHI left refs (`0005`,
`0004`) as their current loop-carried inputs, not to the `IR_PHI` refs (`0012`,
`0013`). This is necessary because PHIs are emitted after the loop body and are
not SSA-dominating definitions for earlier body instructions.

The PHI instruction itself is metadata for the backedge:

- `op1`/left ref is the loop-carried local at the head of the next body
  iteration.
- `op2`/right ref is the value computed by the just-finished body iteration.
- The PHI result ref is not what ordinary same-trace body instructions use.

The native assembler confirms this interpretation. It assembles backwards, so
it sees bottom PHIs first. `asm_phi()` records the right operand register/spill
state and associates it with the left operand; `asm_loop()` then shuffles the
right values into the left locations at the loop boundary:

- `src/lj_asm.c:1728-1762`: sets up right PHI references.
- `src/lj_asm.c:1573-1647`: shuffles PHI registers for the backedge.
- `src/lj_asm.c:1767-1787`: handles `IR_LOOP` as the loop middle/backedge
  point.

Side traces after loop exits consume interpreter slots from snapshots, not PHI
refs. For example, after the numeric loop above exits, the stitched print trace
starts with `SLOAD #2 PI`. Semantically that slot contains the loop-carried
value from the parent exit snapshot; syntactically it is not an `IR_PHI` ref in
the child trace.

## Question 2: minimal structured-Wasm lowering

For the current positive numeric-for subset, the minimal semantic lowering is:

1. Pre-scan the trace:
   - require exactly one `IR_LOOP`
   - require loop PHIs to be the tail instructions after trimming trailing
     `NOP`/`RENAME` (the WASM assembler already trims these at
     `src/lj_asm.c:37-44`)
   - collect `loopref`, first PHI ref, and PHI count
2. Emit all instructions before `IR_LOOP` as the pre-roll. This initializes the
   left PHI refs, e.g. `0005` and `0004`, to the state after one recorded
   iteration.
3. Emit a structured loop for refs between `IR_LOOP + 1` and the first PHI:

```text
loop
  lower copied loop body instructions
  for each PHI: tmp_phi_ref = op2
  for each PHI: op1_local = tmp_phi_ref
  br 0
end
unreachable
```

The two-pass PHI update is important. It preserves simultaneous PHI assignment
and avoids breaking cycles. The existing local allocated for each PHI ref can be
used as the temporary.

This makes post-`IR_LOOP` uses of `op1` work naturally: those refs are mutable
Wasm locals. Before the first Wasm loop iteration they hold the pre-roll values;
after each backedge they are overwritten with the previous iteration's `op2`
values.

Guard lowering can initially keep the current shape:

```text
if guard_fails
  i32.const exitno
  return
end
```

That matches the current host contract, where the trace module returns an exit
status and the host/interpreter side handles restoration/linking. Numeric loop
termination is just the loop-body guard returning the loop-exit snapshot number.

The current scaffold is only a valid-module approximation:

- `src/lj_wasm_jit.c:725-727` lowers `IR_LOOP` as `nop`.
- `src/lj_wasm_jit.c:534-540` lowers `IR_PHI` as `phi_ref = op1`.
- `src/lj_wasm_jit.c:347-351` checks only PHI `op1` support, not `op2`.

So it can validate and run fixture modules, but it does not yet model the
LuaJIT backedge.

## Question 3: tests/fixtures to add first

Recommended first tests, in order:

1. Static Wasm shape test for the existing synthetic numeric-for fixture.
   Assert the lowered module contains a real `loop` and `br 0`, and no longer
   uses `nop` as the `IR_LOOP` lowering. Also assert both PHI right operands are
   consumed before the branch.

2. Runtime backedge test that distinguishes one copied body execution from a
   real loop. A good fixture is a counted numeric loop with a visible side
   effect, e.g. the existing array-store shape, run with enough iterations that
   two straight-line iterations cannot pass. Check memory after execution to
   prove multiple backedges ran.

3. IR-shape fixture/check using `jit.util.traceir()` or `-jdump=irs` for:
   - positive numeric `for`: `LE`, int counter PHI, num accumulator PHI
   - numeric `while`: body uses pre-PHI refs and may leave trailing `RENAME`
   - later, negative-step numeric `for`: `GE`

4. PHI simultaneity fixture. A synthetic two-PHI cycle is enough to ensure the
   lowering uses the two-pass temp update instead of sequentially clobbering
   left operands.

5. Tighten the whitelist before enabling semantics:
   - reject traces where PHIs are not at the tail
   - require PHI `op1` refs to be before or at the pre-roll/body boundary as
     expected
   - require PHI `op2` refs to type-check, and generally be available from the
     loop body or constants

## Commands run

Native IR dumps:

```sh
LUA_PATH='src/?.lua;;' src/luajit -jdump=irs -e 'jit.opt.start("hotloop=1","hotexit=1") local s=0 for i=1,3 do s=s+i end print(s)'
LUA_PATH='src/?.lua;;' src/luajit -jdump=irs -e 'jit.opt.start("hotloop=1","hotexit=1") local s=0 for i=1,4 do s=s+i end local y=s*2 print(y)'
LUA_PATH='src/?.lua;;' src/luajit -jdump=irs -e 'jit.opt.start("hotloop=1","hotexit=1") local i=0 local s=0 while i < 4 do i=i+1 s=s+i end print(s)'
```

Validation:

```sh
make -C wasm/tools check
python3 wasm/tools/wasm_trace_runtime_check.py /tmp/lj-wasm-numeric-for-trace.wasm
wasm-objdump -d /tmp/lj-wasm-numeric-for-trace.wasm
```

`make -C wasm/tools check` passed. It produced local tool binaries; I removed
them afterwards with `make -C wasm/tools clean`.
