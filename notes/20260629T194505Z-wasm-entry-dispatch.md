# 20260629T194505Z - WASM trace entry dispatch

- Branch: `wasm`
- Requested base commit: `fd996a67`
- Observed HEAD: `fd996a6718a40e294ea6610d8641c4c1022b5e62`
- Scope: inspection only. No source files were changed by this slice.

## Short answer

The future WASM VM implementation of the `BC_JLOOP` bytecode dispatch path must
eventually call `lj_wasm_host_jit_enter()`. Native VMs enter compiled traces by
loading `GCtrace->mcode`, saving the current interpreter base, and jumping to the
machine-code pointer. The WASM assembler path deliberately makes `GCtrace->mcode`
a one-byte bookkeeping token and stores the real executable host object in
`GCtrace->wasmjit`, so the WASM `BC_JLOOP` equivalent must load that handle and
call the host entry wrapper instead of jumping through `mcode`.

The call should use:

- `handle`: `(LJWasmHostHandle)T->wasmjit`, where `T = traceref(J, traceno)`.
- `lua_state`: the current `lua_State *L`.
- `base`: the current interpreter `BASE` / `L->base` (`TValue *`).
- `exitno`: `0` for ordinary direct entry from `BC_JLOOP` until the backend
  gives this field a precise non-zero meaning for patched side-exit entry.

This belongs in the future `vm_wasm64`/generated-C VM trace-entry dispatch helper,
not in `lj_trace_hot()`, `lj_trace_ins()`, `lj_dispatch_update()`,
`lj_trace_exit()`, or `lj_asm_trace()`.

## Current WASM host/JIT surface

- `src/lj_wasm_host.h:43-50` defines `LJWasmJITModule` with module bytes, trace
  number, entry export/function index, exit trampoline export/function index, and
  flags.
- `src/lj_wasm_host.h:70-77` declares the JIT host API:
  `lj_wasm_host_jit_compile()`, `lj_wasm_host_jit_free()`,
  `lj_wasm_host_jit_enter(handle, lua_state, base, exitno)`, and
  `lj_wasm_host_jit_patch_exit()`.
- `src/lj_wasm_host.c:75-95` is only a thin wrapper over the raw imports. In
  particular, `lj_wasm_host_jit_enter()` forwards to
  `lj_wasm_import_jit_enter(handle, lua_state, base, exitno)`.
- `src/lj_wasm_jit.c:23-32` documents the placeholder trace module signature:
  `entry(i64 lua_state, i64 base, i32 exitno) -> i32`. `src/lj_wasm_jit.c:35-65`
  emits that export and returns `LJ_WASM_JIT_STATUS_NYI` (`-2`).
- `src/lj_wasm_jit.c:79-100` fills `LJWasmJITModule` with `entry = 0`,
  `exit = 0`, and calls `lj_wasm_host_jit_compile()`.
- `src/lj_asm.c:69-96` is the current WASM trace assembly branch. It trims NOPs,
  allocates the trace, reserves a one-byte `mcode` token, calls
  `lj_wasm_jit_compile_nyi()`, then stores the returned host handle in both
  `T->wasmjit` and `J->curfinal->wasmjit`.
- `src/lj_jit.h:250-289` adds `void *wasmjit` to `GCtrace` under
  `LJ_TARGET_WASM`.
- `src/lj_trace.c:175-193` frees `T->wasmjit` through
  `lj_wasm_host_jit_free()` during trace cleanup.
- `wasm/host/wasmtime/include/lj_wasmtime_host.h:79-88` mirrors the host hook
  signatures, including `LJWasmtimeJITEnterFn(ctx, handle, lua_state, base,
  exitno)`.
- `wasm/host/wasmtime/src/lj_wasmtime_host.c:173-183` validates only that
  `handle != NULL`, delegates to the installed `jit_enter` hook, and otherwise
  returns `LJ_WASM_HOST_NYI`.
- `wasm/host/wasmtime/README.md:36-45` is important for a real host: guest
  pointers must be treated as guest memory offsets/handles, not directly cast to
  native pointers.

`rg` found no runtime caller of `lj_wasm_host_jit_enter()` today other than the
wrapper/import scaffolds. That is expected while the WASM VM backend is gated.

## Existing trace entry model

- `src/lj_dispatch.c:129-176` installs hotcounting dispatch entries when JIT is
  on and not recording. This decides when the VM calls into recorder callbacks;
  it does not enter compiled traces.
- `src/lj_trace.c:790-803` shows `lj_trace_hot()` only starts recording a root
  trace after a hot loop/call. It is not the compiled-entry path.
- `src/lj_trace.c:507-569` is where compiled traces become reachable:
  root traces patch their starting bytecode to `BC_JLOOP`/`BC_JFUNCF`
  (`:516-536`), side traces patch a parent exit with `lj_asm_patchexit()`
  (`:537-540`), and stitched traces patch the previous trace link (`:555-560`).
- Existing DynASM VMs enter traces from `BC_JLOOP` by loading the trace object,
  loading `GCtrace->mcode`, storing `jit_base`, saving `L`, and jumping:
  - `src/vm_x64.dasc:4626-4658`
  - `src/vm_x86.dasc:5428-5460`
  - `src/vm_arm64.dasc:3936-3953`
  - `src/vm_arm.dasc:4587-4597`
  - `src/vm_ppc.dasc:5909-5923`
  - `src/vm_mips.dasc:5240-5253`
  - `src/vm_mips64.dasc:5454-5466`
- Trace stitching routes already funnel back to `BC_JLOOP` when a linked trace is
  present, for example `src/vm_x64.dasc:2335-2369`,
  `src/vm_arm64.dasc:1942-1972`, and `src/vm_ppc.dasc:2885-2925`.
- Function-header dispatch can also jump to `BC_JLOOP`; see
  `src/vm_x64.dasc:4677-4703`.
- `src/lj_vm.h:54-64` declares the assembly VM recording/hook targets and trace
  exit labels (`lj_vm_exit_handler`, `lj_vm_exit_interp`) that each backend must
  provide. `src/lj_vm_wasm.c:31-46` marks `lj_vm_record`,
  `lj_vm_exit_handler`, and `lj_vm_exit_interp` as JIT-facing WASM VM symbols.

So the exact semantic insertion point for WASM trace entry is the WASM backend's
implementation of the native `BC_JLOOP` blocks above.

## Entry/exit state expectations

Native trace entry does a few things around the direct jump that the WASM path
must preserve in some form:

- `L->base` must reflect the current interpreter base before entering the trace.
- `global_State.jit_base` is the current JIT `L->base` or NULL
  (`src/lj_obj.h:659-660`). Native entry stores it before the jump; native exit
  handlers read and then clear it, for example `src/vm_x64.dasc:2438-2449` and
  `src/vm_x64.dasc:2488-2497`.
- `J->L` needs to name the current Lua state before any exit path calls
  `lj_trace_exit()`; the native exit handlers write it before calling
  `lj_trace_exit()` (`src/vm_x64.dasc:2438-2449`,
  `src/vm_arm64.dasc:2039-2056`).
- `src/lj_target_wasm.h:89-99` defines a WASM `ExitState` but does not define
  `EXITSTATE_PCREG`. Therefore `src/lj_trace.c:913-927` will not recover
  `J->parent` from a machine-code PC for WASM; the WASM exit bridge must set
  `J->parent` and `J->exitno` explicitly before calling `lj_trace_exit()`.

Do not feed a raw `LJ_WASM_HOST_NYI`/`LJ_WASM_HOST_ERR` return from
`lj_wasm_host_jit_enter()` into an existing native-style `vm_exit_interp` path as
if it were `MULTRES` or a negated Lua error. The placeholder trace export returns
`-2`, which is a host/JIT status in the WASM contract, not a native trace-exit
result.

## Safe scaffolding before a real VM/backend

Safe now or as a near-term source change while the target remains gated:

- Host-side `jit_enter` hook tests that instantiate the placeholder module,
  call its `"entry"` export with dummy guest pointer values, and assert the
  returned `i32` is `LJ_WASM_HOST_NYI`/`LJ_WASM_JIT_STATUS_NYI`.
- More validation in the Wasmtime scaffold around guest pointer translation and
  opaque handle tables. Keep host handles guest-visible IDs/table indexes, as
  documented in `wasm/host/wasmtime/README.md:36-45`.
- A future WASM VM helper with a narrow contract, for example:
  `vm_wasm_trace_enter(lua_State *L, TValue *base, TraceNo traceno)`, whose first
  version validates `T->wasmjit`, sets the same interpreter/JIT state native
  entry sets, calls `lj_wasm_host_jit_enter(handle, L, base, 0)`, clears
  `jit_base` on fallback, and dispatches `T->startins` if the host returns NYI.
- Keeping `GCtrace->mcode` as the token/range used by existing trace bookkeeping
  while treating `GCtrace->wasmjit` as the executable object.
- Keeping `lj_asm_patchexit()`'s WASM branch as the side-exit linking mechanism:
  `src/lj_asm.c:98-112` already maps parent/child trace handles into
  `lj_wasm_host_jit_patch_exit(from, exitno, to)`.

Not safe until the WASM VM/exit bridge is defined:

- Calling `lj_wasm_host_jit_enter()` from `lj_trace_hot()` or `lj_trace_ins()`.
  Those functions are recorder entry points and do not have the same post-patch
  execution state as `BC_JLOOP`.
- Treating `GCtrace->mcode` as executable code on WASM. It is currently a token.
- Letting a compiled WASM trace return arbitrary native `vm_exit_interp` values
  without a WASM-specific status/result contract.
- Calling `lj_trace_exit()` from host code without constructing a valid
  `ExitState` and setting `J->parent`, `J->exitno`, `J->L`, `L->base`, and
  `jit_base` consistently.

## Suggested next integration steps

1. Define the WASM trace-entry helper contract in the VM backend design. Its input
   should be `L`, current `base`, and the `traceno` decoded from `BC_JLOOP`.
2. In the first executable WASM VM, implement `BC_JLOOP` by loading
   `traceref(J, traceno)->wasmjit` and calling
   `lj_wasm_host_jit_enter(handle, L, base, 0)`.
3. Define the return contract separately from native `vm_exit_interp`: distinguish
   host status (`OK`, `ERR`, `NYI`), trace exit/interpreter continuation, and Lua
   error propagation.
4. Add a WASM trace-exit bridge that fills `ExitState`, sets `J->parent` and
   `J->exitno`, calls `lj_trace_exit(J, &ex)`, and maps the result back to the
   WASM VM dispatcher.
5. Extend the Wasmtime host scaffold so `jit_compile` records the module entry
   export from `LJWasmJITModule.entry`, `jit_enter` invokes that export with
   `(lua_state, base, exitno)`, and `jit_patch_exit` records the parent-exit to
   child-handle continuation.
6. Only after the interpreter VM is working, replace `lj_wasm_jit_compile_nyi()`
   with real IR-to-WASM lowering. The placeholder module is sufficient to exercise
   compile/free/enter plumbing and fallback behavior.
