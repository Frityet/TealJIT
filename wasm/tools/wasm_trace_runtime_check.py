#!/usr/bin/env python3
"""Run a lowered numeric trace module with an imported memory64 memory."""

import struct
import sys

try:
    import wasmtime
except ImportError as exc:
    raise SystemExit(
        "python wasmtime package is required; install with "
        "`python3 -m pip install wasmtime`"
    ) from exc

LJ_TISNUM_TAG_HI = 0xFFF90000


def i32_result(value):
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def write_f64(memory, store, offset, value):
    memory.write(store, struct.pack("<d", value), offset)


def write_i32_tvalue(memory, store, offset, value):
    raw = (LJ_TISNUM_TAG_HI << 32) | (value & 0xFFFFFFFF)
    memory.write(store, raw.to_bytes(8, "little"), offset)


def instantiate(path):
    config = wasmtime.Config()
    config.wasm_memory64 = True
    engine = wasmtime.Engine(config)
    store = wasmtime.Store(engine)
    module = wasmtime.Module(engine, open(path, "rb").read())
    memory_type = wasmtime.MemoryType(wasmtime.Limits(1, None), is_64=True)
    memory = wasmtime.Memory(store, memory_type)
    instance = wasmtime.Instance(store, module, [memory])
    entry = instance.exports(store)["entry"]
    return store, memory, entry


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: wasm_trace_runtime_check.py TRACE_MODULE.wasm")

    store, memory, entry = instantiate(argv[1])
    base = 1024
    exit_state = 4096

    write_f64(memory, store, base + 0, 0.0)  # accumulator SLOAD #2
    write_f64(memory, store, base + 8, 1.0)  # loop index SLOAD #3
    ok = i32_result(entry(store, 0, base, exit_state, 0))
    if ok != 2:
        raise SystemExit(f"expected loop guard failure status 2, got {ok}")

    write_i32_tvalue(memory, store, base + 0, 0)
    failed = i32_result(entry(store, 0, base, exit_state, 0))
    if failed != 0:
        raise SystemExit(f"expected numeric guard failure status 0, got {failed}")

    write_f64(memory, store, base + 0, 0.0)
    write_f64(memory, store, base + 8, 101.0)
    failed_limit = i32_result(entry(store, 0, base, exit_state, 0))
    if failed_limit != 1:
        raise SystemExit(
            f"expected first loop guard failure status 1, got {failed_limit}"
        )


if __name__ == "__main__":
    main(sys.argv)
