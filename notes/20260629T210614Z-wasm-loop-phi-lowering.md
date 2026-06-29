# 20260629T210614Z - WASM loop and PHI lowering

Base commit: 8b4344efe6bde00b5a4459f93dfdee263abce553

This slice lowers LuaJIT `IR_LOOP` into a structured Wasm `loop` and turns
trailing `IR_PHI` instructions into explicit loop-backedge copies. The lowering
uses the same convention as LuaJIT's native backend: PHI instructions are below
the loop body, the body reads the left/pre-loop refs, and the backedge copies
right/body refs back into those left refs before branching to the loop label.

To avoid clobbering across multiple PHIs, the backedge uses PHI locals as
temporaries in two phases:

1. Evaluate every right PHI operand into its PHI ref local.
2. Copy every PHI ref local into the left operand local.
3. Emit `br 0` to continue the Wasm loop.

The first Wasmtime runtime smoke now executes the numeric-for trace as a real
loop. The normal path runs until the body `LE` guard fails and returns exit `2`;
the earlier type and pre-loop limit failures still return exits `0` and `1`.

Current limitation: PHI right operands that are themselves PHI refs are rejected
for now. That avoids silently mis-lowering rarer recurrence graphs until the
backend has fuller PHI-cycle coverage and exit-state restoration.
