# 20260629T201531Z - WASM trace load regression check

Base commit: 6ed0f475

Hardened `wasm/tools` so the synthetic lowered trace test checks for the
observable SLOAD-lowering features, not just Wasm validity. `trace-ir-module`
now requires:

- a memory64 import shown by `wasm-objdump -x` as `i64 <- env.memory`
- an `i32.load` in the trace body
- an `f64.load` in the trace body

This catches regressions where the trace builder silently falls back to
placeholder constants while still producing a valid module.

Validation:

- `make -C wasm/tools clean`
- `make -C wasm/tools check`
