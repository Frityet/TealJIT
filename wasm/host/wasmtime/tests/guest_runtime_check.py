#!/usr/bin/env python3
"""Exercise Wasmtime guest-memory FFI wrappers through an actual memory64 module."""

from __future__ import annotations

import ctypes
import struct
import sys
from pathlib import Path

try:
    import wasmtime
except ImportError as exc:
    raise SystemExit(
        "python wasmtime package is required for guest-runtime-check"
    ) from exc


OK = 0
ERR = -1

HANDLE_MAX = 1024
FFI_MAX_ARGS = 16
FFI_CALL_ABI_VERSION = 1

SCALAR_F64 = 4
LOC_FPR = 2

LIB_NAME = 1024
SYMBOL_NAME = 1040
LIB_HANDLE = 8
SYMBOL_HANDLE = 16
CCALL_FRAME = 128
CALL_DESC = 256


class LJWasmFFISig(ctypes.Structure):
    _fields_ = [
        ("ctypeid", ctypes.c_uint32),
        ("nargs", ctypes.c_uint16),
        ("flags", ctypes.c_uint8),
        ("rettype", ctypes.c_uint8),
    ]


class LJWasmFFISlot(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_uint8),
        ("loc", ctypes.c_uint8),
        ("flags", ctypes.c_uint16),
        ("offset", ctypes.c_uint32),
        ("size", ctypes.c_uint32),
    ]


class LJWasmFFICall(ctypes.Structure):
    _fields_ = [
        ("sig", LJWasmFFISig),
        ("abi_version", ctypes.c_uint32),
        ("ccall_size", ctypes.c_uint32),
        ("func_offset", ctypes.c_uint32),
        ("reserved", ctypes.c_uint32),
        ("ret", LJWasmFFISlot),
        ("args", LJWasmFFISlot * FFI_MAX_ARGS),
    ]


class LJWasmtimeGuestMemory(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_uint64),
    ]


class LJWasmtimeHandleTable(ctypes.Structure):
    _fields_ = [("slots", ctypes.c_void_p * HANDLE_MAX)]


class LJWasmtimeGuestContext(ctypes.Structure):
    _fields_ = [
        ("memory", LJWasmtimeGuestMemory),
        ("handles", LJWasmtimeHandleTable),
    ]


def load_host(path: Path):
    lib = ctypes.CDLL(str(path))
    ctx_p = ctypes.POINTER(LJWasmtimeGuestContext)
    lib.lj_wasmtime_guest_ffi_load.argtypes = [
        ctx_p,
        ctypes.c_uint64,
        ctypes.c_int,
        ctypes.c_uint64,
    ]
    lib.lj_wasmtime_guest_ffi_load.restype = ctypes.c_int
    lib.lj_wasmtime_guest_ffi_symbol.argtypes = [
        ctx_p,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.lj_wasmtime_guest_ffi_symbol.restype = ctypes.c_int
    lib.lj_wasmtime_guest_ffi_call.argtypes = [
        ctx_p,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.lj_wasmtime_guest_ffi_call.restype = ctypes.c_int
    lib.lj_wasmtime_guest_ffi_unload.argtypes = [ctx_p, ctypes.c_uint64]
    lib.lj_wasmtime_guest_ffi_unload.restype = None
    return lib


def call_desc_bytes() -> bytes:
    call = LJWasmFFICall()
    call.sig.nargs = 1
    call.sig.rettype = SCALAR_F64
    call.abi_version = FFI_CALL_ABI_VERSION
    call.ccall_size = 64
    call.func_offset = 0
    call.ret.type = SCALAR_F64
    call.ret.loc = LOC_FPR
    call.ret.offset = 32
    call.ret.size = 8
    call.args[0].type = SCALAR_F64
    call.args[0].loc = LOC_FPR
    call.args[0].offset = 16
    call.args[0].size = 8
    return ctypes.string_at(ctypes.byref(call), ctypes.sizeof(call))


def i32(value: int) -> int:
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def write_u64(memory: wasmtime.Memory, store: wasmtime.Store, offset: int, value: int) -> None:
    memory.write(store, struct.pack("<Q", value), offset)


def read_u64(memory: wasmtime.Memory, store: wasmtime.Store, offset: int) -> int:
    return struct.unpack("<Q", memory.read(store, offset, offset + 8))[0]


def write_f64(memory: wasmtime.Memory, store: wasmtime.Store, offset: int, value: float) -> None:
    memory.write(store, struct.pack("<d", value), offset)


def read_f64(memory: wasmtime.Memory, store: wasmtime.Store, offset: int) -> float:
    return struct.unpack("<d", memory.read(store, offset, offset + 8))[0]


def guest_module() -> bytes:
    return wasmtime.wat2wasm(
        f"""
        (module
          (import "env" "lj_wasm_import_ffi_load"
            (func $ffi_load (param i64 i32 i64) (result i32)))
          (import "env" "lj_wasm_import_ffi_symbol"
            (func $ffi_symbol (param i64 i64 i64) (result i32)))
          (import "env" "lj_wasm_import_ffi_call"
            (func $ffi_call (param i64 i64 i64 i64) (result i32)))
          (import "env" "lj_wasm_import_ffi_unload"
            (func $ffi_unload (param i64)))
          (memory (export "memory") i64 1)
          (data (i64.const {LIB_NAME}) "libm.so.6\\00")
          (data (i64.const {SYMBOL_NAME}) "cos\\00")
          (func (export "load") (result i32)
            i64.const {LIB_NAME}
            i32.const 0
            i64.const {LIB_HANDLE}
            call $ffi_load)
          (func (export "symbol") (result i32)
            i64.const {LIB_HANDLE}
            i64.load
            i64.const {SYMBOL_NAME}
            i64.const {SYMBOL_HANDLE}
            call $ffi_symbol)
          (func (export "place_symbol")
            i64.const {CCALL_FRAME}
            i64.const {SYMBOL_HANDLE}
            i64.load
            i64.store)
          (func (export "call") (result i32)
            i64.const 1
            i64.const 2
            i64.const {CCALL_FRAME}
            i64.const {CALL_DESC}
            call $ffi_call)
          (func (export "unload")
            i64.const {LIB_HANDLE}
            i64.load
            call $ffi_unload))
        """
    )


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        raise SystemExit("usage: guest_runtime_check.py LIBLJ_WASMTIME_HOST.so")

    host = load_host(Path(argv[1]).resolve())
    ctx = LJWasmtimeGuestContext()

    config = wasmtime.Config()
    config.wasm_memory64 = True
    engine = wasmtime.Engine(config)
    store = wasmtime.Store(engine)
    linker = wasmtime.Linker(engine)

    def update_context(caller: wasmtime.Caller) -> None:
        memory = caller.get("memory")
        if memory is None:
            raise RuntimeError("guest memory export not found")
        ctx.memory.data = memory.data_ptr(caller)
        ctx.memory.size = memory.data_len(caller)

    def ffi_load(caller, name_ptr, global_flag, handle_out):
        update_context(caller)
        return host.lj_wasmtime_guest_ffi_load(
            ctypes.byref(ctx), name_ptr, global_flag, handle_out
        )

    def ffi_symbol(caller, handle, name_ptr, symbol_out):
        update_context(caller)
        return host.lj_wasmtime_guest_ffi_symbol(
            ctypes.byref(ctx), handle, name_ptr, symbol_out
        )

    def ffi_call(caller, cts, ct, cc, call):
        update_context(caller)
        return host.lj_wasmtime_guest_ffi_call(
            ctypes.byref(ctx), cts, ct, cc, call
        )

    def ffi_unload(caller, handle):
        update_context(caller)
        host.lj_wasmtime_guest_ffi_unload(ctypes.byref(ctx), handle)

    i64 = wasmtime.ValType.i64()
    i32t = wasmtime.ValType.i32()
    linker.define_func(
        "env",
        "lj_wasm_import_ffi_load",
        wasmtime.FuncType([i64, i32t, i64], [i32t]),
        ffi_load,
        access_caller=True,
    )
    linker.define_func(
        "env",
        "lj_wasm_import_ffi_symbol",
        wasmtime.FuncType([i64, i64, i64], [i32t]),
        ffi_symbol,
        access_caller=True,
    )
    linker.define_func(
        "env",
        "lj_wasm_import_ffi_call",
        wasmtime.FuncType([i64, i64, i64, i64], [i32t]),
        ffi_call,
        access_caller=True,
    )
    linker.define_func(
        "env",
        "lj_wasm_import_ffi_unload",
        wasmtime.FuncType([i64], []),
        ffi_unload,
        access_caller=True,
    )

    instance = linker.instantiate(store, wasmtime.Module(engine, guest_module()))
    exports = instance.exports(store)
    memory = exports["memory"]

    memory.write(store, call_desc_bytes(), CALL_DESC)
    memory.write(store, bytes(64), CCALL_FRAME)
    write_f64(memory, store, CCALL_FRAME + 16, 0.0)

    assert i32(exports["load"](store)) == OK
    lib_handle = read_u64(memory, store, LIB_HANDLE)
    assert lib_handle != 0
    assert i32(exports["symbol"](store)) == OK
    symbol_handle = read_u64(memory, store, SYMBOL_HANDLE)
    assert symbol_handle != 0

    exports["place_symbol"](store)
    assert read_u64(memory, store, CCALL_FRAME) == symbol_handle
    assert i32(exports["call"](store)) == OK
    assert read_f64(memory, store, CCALL_FRAME + 32) == 1.0
    assert read_u64(memory, store, CCALL_FRAME) == symbol_handle

    exports["unload"](store)
    assert i32(exports["call"](store)) == ERR
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
