#!/usr/bin/env python3
"""Exercise Wasmtime guest-memory wrappers through actual memory64 modules."""

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
JIT_ENTER_STATUS = 77

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

JIT_BYTES = 1024
JIT_MODULE_A = 2048
JIT_MODULE_B = 2112
JIT_HANDLE_A = 64
JIT_HANDLE_B = 72

JIT_F_IR_LOWERED = 0x00000001
JIT_F_IMPORT_ENV_MEMORY = 0x00000002
JIT_MEMORY_F_64 = 0x00000001


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


class LJWasmJITModule(ctypes.Structure):
    _fields_ = [
        ("bytes", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_size_t),
        ("trace", ctypes.c_uint32),
        ("entry", ctypes.c_uint32),
        ("exit", ctypes.c_uint32),
        ("flags", ctypes.c_uint32),
        ("memory_min", ctypes.c_uint64),
        ("memory_max", ctypes.c_uint64),
        ("memory_flags", ctypes.c_uint32),
        ("reserved", ctypes.c_uint32),
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


class LJWasmtimeHostHooks(ctypes.Structure):
    _fields_ = [
        ("ctx", ctypes.c_void_p),
        ("ffi_load", ctypes.c_void_p),
        ("ffi_unload", ctypes.c_void_p),
        ("ffi_symbol", ctypes.c_void_p),
        ("ffi_call", ctypes.c_void_p),
        ("ffi_callback_new", ctypes.c_void_p),
        ("ffi_callback_slot", ctypes.c_void_p),
        ("ffi_callback_free", ctypes.c_void_p),
        ("jit_compile", ctypes.c_void_p),
        ("jit_free", ctypes.c_void_p),
        ("jit_enter", ctypes.c_void_p),
        ("jit_patch_exit", ctypes.c_void_p),
    ]


JITCompileCB = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_void_p,
    ctypes.POINTER(LJWasmJITModule),
    ctypes.POINTER(ctypes.c_void_p),
)
JITFreeCB = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_void_p)
JITEnterCB = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_uint32,
)
JITPatchCB = ctypes.CFUNCTYPE(
    ctypes.c_int,
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_uint32,
    ctypes.c_void_p,
)


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
    lib.lj_wasmtime_guest_jit_compile.argtypes = [
        ctx_p,
        ctypes.c_uint64,
        ctypes.c_uint64,
    ]
    lib.lj_wasmtime_guest_jit_compile.restype = ctypes.c_int
    lib.lj_wasmtime_guest_jit_free.argtypes = [ctx_p, ctypes.c_uint64]
    lib.lj_wasmtime_guest_jit_free.restype = None
    lib.lj_wasmtime_guest_jit_enter.argtypes = [
        ctx_p,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint64,
        ctypes.c_uint32,
    ]
    lib.lj_wasmtime_guest_jit_enter.restype = ctypes.c_int
    lib.lj_wasmtime_guest_jit_patch_exit.argtypes = [
        ctx_p,
        ctypes.c_uint64,
        ctypes.c_uint32,
        ctypes.c_uint64,
    ]
    lib.lj_wasmtime_guest_jit_patch_exit.restype = ctypes.c_int
    lib.lj_wasmtime_host_set_hooks.argtypes = [
        ctypes.POINTER(LJWasmtimeHostHooks)
    ]
    lib.lj_wasmtime_host_set_hooks.restype = None
    lib.lj_wasmtime_host_clear_hooks.argtypes = []
    lib.lj_wasmtime_host_clear_hooks.restype = None
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


def jit_module_bytes() -> bytes:
    return wasmtime.wat2wasm("(module (func (export \"entry\") (result i32) i32.const 0))")


def jit_desc_bytes(bytes_offset: int, size: int, trace: int) -> bytes:
    module = LJWasmJITModule()
    module.bytes = ctypes.cast(bytes_offset, ctypes.POINTER(ctypes.c_uint8))
    module.size = size
    module.trace = trace
    module.entry = 0
    module.exit = 0
    module.flags = JIT_F_IR_LOWERED | JIT_F_IMPORT_ENV_MEMORY
    module.memory_min = 1
    module.memory_max = 0
    module.memory_flags = JIT_MEMORY_F_64
    return ctypes.string_at(ctypes.byref(module), ctypes.sizeof(module))


def jit_guest_module() -> bytes:
    return wasmtime.wat2wasm(
        f"""
        (module
          (import "env" "lj_wasm_import_jit_compile"
            (func $jit_compile (param i64 i64) (result i32)))
          (import "env" "lj_wasm_import_jit_free"
            (func $jit_free (param i64)))
          (import "env" "lj_wasm_import_jit_enter"
            (func $jit_enter (param i64 i64 i64 i64 i32) (result i32)))
          (import "env" "lj_wasm_import_jit_patch_exit"
            (func $jit_patch_exit (param i64 i32 i64) (result i32)))
          (memory (export "memory") i64 1)
          (func (export "compile_a") (result i32)
            i64.const {JIT_MODULE_A}
            i64.const {JIT_HANDLE_A}
            call $jit_compile)
          (func (export "compile_b") (result i32)
            i64.const {JIT_MODULE_B}
            i64.const {JIT_HANDLE_B}
            call $jit_compile)
          (func (export "enter_a") (result i32)
            i64.const {JIT_HANDLE_A}
            i64.load
            i64.const 16
            i64.const 32
            i64.const 48
            i32.const 9
            call $jit_enter)
          (func (export "patch_a_to_b") (result i32)
            i64.const {JIT_HANDLE_A}
            i64.load
            i32.const 7
            i64.const {JIT_HANDLE_B}
            i64.load
            call $jit_patch_exit)
          (func (export "free_a")
            i64.const {JIT_HANDLE_A}
            i64.load
            call $jit_free))
        """
    )


def install_jit_hooks(host):
    trace_a = ctypes.c_int(100)
    trace_b = ctypes.c_int(200)
    trace_a_ptr = ctypes.cast(ctypes.pointer(trace_a), ctypes.c_void_p).value
    trace_b_ptr = ctypes.cast(ctypes.pointer(trace_b), ctypes.c_void_p).value
    state = {
        "compile": 0,
        "free": 0,
        "enter": 0,
        "patch": 0,
        "last_trace": 0,
        "last_exitno": 0,
    }

    @JITCompileCB
    def jit_compile(_ctx, module_p, handle_out):
        module = module_p.contents
        module_bytes = ctypes.string_at(module.bytes, module.size)
        assert module_bytes.startswith(b"\x00asm")
        assert module.memory_min == 1
        assert module.memory_flags == JIT_MEMORY_F_64
        state["compile"] += 1
        state["last_trace"] = module.trace
        handle_out[0] = trace_a_ptr if state["compile"] == 1 else trace_b_ptr
        return OK

    @JITFreeCB
    def jit_free(_ctx, handle):
        assert int(handle) == trace_a_ptr
        state["free"] += 1

    @JITEnterCB
    def jit_enter(_ctx, handle, lua_state, base, exit_state, exitno):
        assert int(handle) == trace_a_ptr
        assert int(lua_state) == 16
        assert int(base) == 32
        assert int(exit_state) == 48
        state["enter"] += 1
        state["last_exitno"] = int(exitno)
        return JIT_ENTER_STATUS

    @JITPatchCB
    def jit_patch(_ctx, from_handle, exitno, to_handle):
        assert int(from_handle) == trace_a_ptr
        assert int(to_handle) == trace_b_ptr
        state["patch"] += 1
        state["last_exitno"] = int(exitno)
        return OK

    hooks = LJWasmtimeHostHooks()
    hooks.jit_compile = ctypes.cast(jit_compile, ctypes.c_void_p).value
    hooks.jit_free = ctypes.cast(jit_free, ctypes.c_void_p).value
    hooks.jit_enter = ctypes.cast(jit_enter, ctypes.c_void_p).value
    hooks.jit_patch_exit = ctypes.cast(jit_patch, ctypes.c_void_p).value
    host.lj_wasmtime_host_set_hooks(ctypes.byref(hooks))
    return state, hooks, (jit_compile, jit_free, jit_enter, jit_patch), (trace_a, trace_b)


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

    state, hooks, callbacks, traces = install_jit_hooks(host)
    jit_ctx = LJWasmtimeGuestContext()

    def update_jit_context(caller: wasmtime.Caller) -> None:
        memory = caller.get("memory")
        if memory is None:
            raise RuntimeError("jit guest memory export not found")
        jit_ctx.memory.data = memory.data_ptr(caller)
        jit_ctx.memory.size = memory.data_len(caller)

    def jit_compile_import(caller, module_ptr, handle_out):
        update_jit_context(caller)
        return host.lj_wasmtime_guest_jit_compile(
            ctypes.byref(jit_ctx), module_ptr, handle_out
        )

    def jit_free_import(caller, handle):
        update_jit_context(caller)
        host.lj_wasmtime_guest_jit_free(ctypes.byref(jit_ctx), handle)

    def jit_enter_import(caller, handle, lua_state, base, exit_state, exitno):
        update_jit_context(caller)
        return host.lj_wasmtime_guest_jit_enter(
            ctypes.byref(jit_ctx), handle, lua_state, base, exit_state, exitno
        )

    def jit_patch_import(caller, from_handle, exitno, to_handle):
        update_jit_context(caller)
        return host.lj_wasmtime_guest_jit_patch_exit(
            ctypes.byref(jit_ctx), from_handle, exitno, to_handle
        )

    jit_linker = wasmtime.Linker(engine)
    jit_linker.define_func(
        "env",
        "lj_wasm_import_jit_compile",
        wasmtime.FuncType([i64, i64], [i32t]),
        jit_compile_import,
        access_caller=True,
    )
    jit_linker.define_func(
        "env",
        "lj_wasm_import_jit_free",
        wasmtime.FuncType([i64], []),
        jit_free_import,
        access_caller=True,
    )
    jit_linker.define_func(
        "env",
        "lj_wasm_import_jit_enter",
        wasmtime.FuncType([i64, i64, i64, i64, i32t], [i32t]),
        jit_enter_import,
        access_caller=True,
    )
    jit_linker.define_func(
        "env",
        "lj_wasm_import_jit_patch_exit",
        wasmtime.FuncType([i64, i32t, i64], [i32t]),
        jit_patch_import,
        access_caller=True,
    )

    jit_instance = jit_linker.instantiate(
        store, wasmtime.Module(engine, jit_guest_module())
    )
    jit_exports = jit_instance.exports(store)
    jit_memory = jit_exports["memory"]
    compiled = jit_module_bytes()
    jit_memory.write(store, compiled, JIT_BYTES)
    jit_memory.write(store, jit_desc_bytes(JIT_BYTES, len(compiled), 11), JIT_MODULE_A)
    jit_memory.write(store, jit_desc_bytes(JIT_BYTES, len(compiled), 12), JIT_MODULE_B)

    assert i32(jit_exports["compile_a"](store)) == OK
    assert read_u64(jit_memory, store, JIT_HANDLE_A) != 0
    assert state["compile"] == 1
    assert state["last_trace"] == 11
    assert i32(jit_exports["compile_b"](store)) == OK
    assert read_u64(jit_memory, store, JIT_HANDLE_B) != 0
    assert state["compile"] == 2
    assert state["last_trace"] == 12
    assert i32(jit_exports["enter_a"](store)) == JIT_ENTER_STATUS
    assert state["enter"] == 1
    assert state["last_exitno"] == 9
    assert i32(jit_exports["patch_a_to_b"](store)) == OK
    assert state["patch"] == 1
    assert state["last_exitno"] == 7
    jit_exports["free_a"](store)
    assert state["free"] == 1
    assert i32(jit_exports["enter_a"](store)) == ERR

    host.lj_wasmtime_host_clear_hooks()
    _keepalive = (hooks, callbacks, traces)
    assert _keepalive
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
