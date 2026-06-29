# 20260629T205454Z - Wasmtime imported-memory runtime smoke

Base commit: 905cc3a1

Installed validation tooling in the container:

```sh
sudo apt-get install -y python3-pip
python3 -m pip install --break-system-packages wasmtime
```

The installed Python binding is `wasmtime 46.0.1`.

Added `wasm/tools/wasm_trace_runtime_check.py` and wired it into
`wasm/tools check`. The new smoke test:

1. builds the numeric-for trace module
2. instantiates it with a real imported memory64 `env.memory`
3. writes LuaJIT-style TValue payloads into that memory
4. calls the trace `entry(lua_state, base, exitno)`
5. verifies the valid inputs return the current fallthrough status `-2`
6. rewrites the accumulator as an integer TValue and verifies the numeric guard
   exits with status `0`

This is not a full host JIT implementation yet, but it proves that the emitted
memory-importing trace modules can be instantiated and executed by Wasmtime when
given a shared memory64 memory object.

Validation:

- `make -C wasm/tools clean && make -C wasm/tools check`
