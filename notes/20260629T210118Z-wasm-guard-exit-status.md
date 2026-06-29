# 20260629T210118Z - WASM guard exit status mapping

Base commit: 94968082374cd4b86361f6ab278286a242c36fb4

This slice replaces the earlier placeholder guard return path where every
lowered WASM guard returned `0`. Trace entries now reserve negative returns for
backend statuses such as `LJ_WASM_JIT_STATUS_NYI`, while non-negative returns
represent LuaJIT trace exit numbers.

For normal traces, `src/lj_wasm_jit.c` maps each guard IR ref to the latest
snapshot whose `SnapShot.ref` is less than or equal to that guard ref. This
preserves the existing LuaJIT convention that `exitno == snapno`. The standalone
developer fixtures do not build snapshot arrays, so they fall back to a
deterministic guard ordinal in emission order.

The Wasmtime memory64 runtime smoke now checks two guard failures: the initial
numeric type guard returns exit `0`, and the first loop-limit guard returns exit
`1`. This keeps the current lowered trace behavior honest until the real WASM
exit-state materialization path lands.
