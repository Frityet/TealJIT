# LuaJIT Native Teal Front-End Integration Notes

Date: 2026-06-29 18:27:53 UTC
Commit: 9d145d2ca3db58493859c495489a0f08f627834f
Short commit: 9d145d2c
Scope: inspection only; no source files edited.

## Current workspace state

- `git status --short` showed untracked `.devcontainer/` and `tl/`.
- No tracked source diffs were observed under `src/` before writing this note.
- This note is the only intended write for this task.

## Loader path and best hook point

The central loading path is compact:

- `src/lj_load.c:62-75` implements `lua_loadx()`. It stores the reader, data, chunk name argument, and mode in a stack `LexState`, then calls `cpparser()` under `lj_vm_cpcall()` and always runs `lj_lex_cleanup()`.
- `src/lj_load.c:28-59` implements `cpparser()`. It calls `lj_lex_setup()`, validates binary/text mode, then dispatches to `lj_bcread(ls)` for bytecode or `lj_parse(ls)` for text at `src/lj_load.c:50`.
- `src/lj_load.c:98-130` implements `luaL_loadfilex()` with a binary file reader; `src/lj_load.c:153-159` implements `luaL_loadbufferx()` with an in-memory string reader.

This makes `cpparser()` the safest single dispatch point for a native Teal front-end:

```c
bc = lj_lex_setup(L, ls);
...
pt = bc ? lj_bcread(ls) : (is_teal ? lj_teal_parse(ls) : lj_parse(ls));
```

Do not bypass `lua_loadx()` unless there is a strong reason. It already centralizes protected execution, GC cleanup, and the legacy/non-native prototype behavior.

Important mode behavior:

- The current mode check distinguishes only bytecode (`b`) from source text (`t`) at `src/lj_load.c:37-49`.
- `W` and `X` in the mode string flip `ls->fr2` for non-native bytecode generation at `src/lj_load.c:43`.
- Other mode characters are effectively ignored once the current category has matched. This is already used by `jit.bcsave`, which passes mode strings such as `btsW`, `btdX`, etc.

Implication: a Teal mode marker can be added, but validation must preserve existing `b/t/s/d/W/X` behavior. Treat Teal as a text submode, not as a new bytecode category.

## Lexer and parser constraints

`src/lj_lex.c` is the Lua lexer, not a general front-end abstraction:

- `lj_lex_setup()` initializes all parser-related stacks and reads the first character at `src/lj_lex.c:397-443`.
- It skips UTF-8 BOM and POSIX shebang lines before checking for bytecode at `src/lj_lex.c:416-430`.
- It rejects bytecode with an extra header for security reasons at `src/lj_lex.c:430-441`.
- `lex_scan()` recognizes Lua identifiers, numbers, strings, comments, punctuation, and Lua keywords at `src/lj_lex.c:291-393`.
- Reserved words are interned globally in `lj_lex_init()` at `src/lj_lex.c:505-514`, which is called from state initialization at `src/lj_state.c:199-202`.

Do not add Teal-only reserved words to the shared Lua reserved-word table unless the lexer is explicitly mode-gated. Making words like `record`, `enum`, `type`, `interface`, `as`, or `is` globally reserved would be a Lua compatibility break.

`src/lj_parse.c` is also tightly coupled:

- `FuncState`, `ExpDesc`, bytecode emitter helpers, scope handling, and many parser helpers are private static implementation details near `src/lj_parse.c:31-146` and throughout the file.
- `lj_parse()` is the only public parser entry in `src/lj_parse.h:12`.
- `lj_parse()` creates the main vararg function, emits a placeholder `BC_FUNCV`, reads the first token, parses the chunk, verifies EOF, and returns a finished `GCproto *` at `src/lj_parse.c:2702-2734`.
- `fs_finish()` allocates and populates the `GCproto`, constants, upvalues, line info, and debug var info at `src/lj_parse.c:1559-1606`.

This argues against a large in-place Teal mode inside `lj_parse.c`. A sibling front-end, e.g. `lj_teal.c`/`lj_teal.h` or `lj_tlex.c`/`lj_tparse.c`, can preserve the existing Lua path and return an ordinary `GCproto *` through the same loader contract.

## CLI option handling

`src/luajit.c` currently has several independent load sites:

- `dofile()` uses `luaL_loadfile()` at `src/luajit.c:183-186`.
- `dostring()` uses `luaL_loadbuffer()` at `src/luajit.c:189-192`.
- REPL line loading uses `luaL_loadbuffer()` at `src/luajit.c:244-260`.
- Script execution uses `luaL_loadfile()` at `src/luajit.c:285-309`.
- `LUA_INIT` can call either file or string loading at `src/luajit.c:505-518`.

Argument parsing is simple and single-character:

- Usage text is hard-coded at `src/luajit.c:78-95`.
- `collectargs()` recognizes `-e`, `-l`, `-j`, `-O`, `-b`, `-i`, `-v`, `-E`, `--`, and `-` at `src/luajit.c:417-460`.
- `runargs()` executes options and delegates `-b` to `dobytecode()` at `src/luajit.c:462-503`.

Minimal CLI exposure choices:

1. Auto-detect Teal only for file chunks with `@*.tl` chunk names. This covers `luajit file.tl`, `loadfile("file.tl")`, and probably `luajit -b file.tl out`, without changing CLI parsing. It does not cover `-e`, stdin, or REPL Teal input.
2. Add an explicit `-T` or similar option. This requires updating usage, `collectargs()`, `runargs()`, and every helper that calls `luaL_loadfile()`/`luaL_loadbuffer()` so the selected source mode reaches the loader. It also needs a decision for `LUA_INIT`, stdin, and REPL behavior.

Long options are not natural in the current parser: `--` is the only dash-dash option and stops option parsing.

## Bytecode save/list integration

`luajit -b` is not implemented in C parser code. `src/luajit.c:390-406` loads the Lua module `jit.bcsave`, then passes the remaining arguments.

`src/jit/bcsave.lua` loads input through the normal public APIs:

- `loadstring(input, nil, ctx.mode)` for `-be` chunks at `src/jit/bcsave.lua:56-59`.
- `load(data, ctx.filename, ctx.mode)` for filename-overridden chunks at `src/jit/bcsave.lua:60-69`.
- `loadfile(input, ctx.mode)` for normal files at `src/jit/bcsave.lua:70-72`.
- It starts with `ctx.mode = "bt"` at `src/jit/bcsave.lua:574-576` and appends deterministic/strip/FR2 flags at `src/jit/bcsave.lua:593-621`.
- It dumps with `string.dump(f, ctx.mode)` at `src/jit/bcsave.lua:550-552`.

If Teal is auto-detected from the input filename or from a mode marker that bcsave can pass through, then `-b` can keep working with minimal changes. If Teal requires a CLI-only flag, `jit.bcsave` will need a matching option and mode propagation.

## Build and amalgamation points

The normal POSIX/MinGW build lists core object files in `src/Makefile`:

- `LJCORE_O` includes `lj_lex.o lj_parse.o lj_bcread.o lj_bcwrite.o lj_load.o` at `src/Makefile:517-530`.
- `LJVMCORE_O` is `$(LJVM_O) $(LJCORE_O)` at `src/Makefile:532`.
- `make amalg` replaces the normal core list with `LJCORE_O=ljamalg.o` at `src/Makefile:636-637`.
- `Makefile.dep` is generated by the `depend` target at `src/Makefile:646-655` and currently has dependency entries for `lj_lex.o`, `lj_load.o`, `lj_parse.o`, and `ljamalg.o` at `src/Makefile.dep:136-146`, `src/Makefile.dep:174-177`, and `src/Makefile.dep:222-244`.

The amalgamation file explicitly includes the core `.c` files:

- `src/ljamalg.c:45-49` includes `lj_lex.c`, `lj_parse.c`, `lj_bcread.c`, `lj_bcwrite.c`, and `lj_load.c`.

Build implications for native Teal files:

- Add new object files to `LJCORE_O`, probably near `lj_lex.o lj_parse.o`.
- Add the new `.c` files to `src/ljamalg.c` in dependency order before `lj_load.c` if `lj_load.c` calls into them.
- Regenerate or update `src/Makefile.dep`.
- Also audit non-Makefile build scripts such as `src/msvcbuild.bat` if Windows builds matter; `src/Makefile` itself points readers there at `src/Makefile:6-8`.

No `buildvm` table updates are needed unless the integration adds new bytecodes, new fast functions, new library definitions, or JIT recorder definitions.

## Bytecode compatibility constraints

The safe target is ordinary LuaJIT `GCproto` and existing bytecode:

- `GCproto` layout and persisted flags are in `src/lj_obj.h:372-413`.
- Bytecode dump format is documented in `src/lj_bcdump.h:14-29`.
- `BCDUMP_VERSION` is currently `2`; the header warns that private bytecode or dump format changes must set it to `0x80` or higher at `src/lj_bcdump.h:36-39`.
- Dump compatibility flags are only BE, STRIP, FFI, and FR2 at `src/lj_bcdump.h:41-49`.
- `bcread_header()` rejects unknown flags, wrong version, and FR2 mismatches at `src/lj_bcread.c:393-419`.
- `bcwrite_header()` writes the current version and known flags only at `src/lj_bcwrite.c:387-407`.
- `bcwrite_proto()` only serializes `PROTO_CHILD`, `PROTO_VARARG`, and `PROTO_FFI` from `pt->flags` at `src/lj_bcwrite.c:342-350`.

Therefore:

- Do not add Teal runtime semantics that require new opcodes, a new prototype layout, or new serialized flags in the first integration.
- Type syntax should erase to existing LuaJIT semantics before bytecode generation.
- The generated prototypes must preserve `LJ_FR2` handling, frame sizes, upvalues, constants, child prototypes, and line/debug info.
- Teal-specific source metadata should not be stored in bytecode unless the project intentionally accepts a private dump format version and compatibility break.

## Minimal safe integration plan

1. Add a feature flag such as `LUAJIT_ENABLE_TEAL` and keep it easy to disable during early integration.
2. Add a narrow internal API, for example `lj_teal_parse(LexState *ls) -> GCproto *`, declared in a new header and called only from `cpparser()`.
3. Start with explicit file detection, e.g. chunk names beginning with `@` and ending in `.tl`, plus an optional mode marker for buffers later. Avoid global content sniffing.
4. Preserve bytecode detection first. `lj_lex_setup()` must still reject bytecode after BOM/shebang exactly as it does now.
5. Keep Teal as a source-text submode under the existing `b/t` mode model. Do not introduce a third loader category.
6. Use a separate lexer/token namespace for Teal-only syntax, or hard-gate any Teal tokens so the existing Lua lexer behavior is unchanged for `.lua` and normal buffers.
7. Generate ordinary `GCproto` objects. The first milestone can erase Teal type syntax to Lua-compatible parsing or use a sibling parser/emitter, but it should not change `lj_bc.h`, `GCproto`, or `lj_bcdump.h`.
8. Add CLI support only after the loader path is stable. Auto `.tl` file loading is the smallest CLI-visible behavior; `-T` can follow if `-e`, stdin, and REPL Teal support are required.
9. Make `jit.bcsave` work either by relying on `.tl` auto-detection or by adding a matching bcsave option that passes the Teal mode marker through `load/loadfile`.
10. Update `src/Makefile`, `src/ljamalg.c`, and `src/Makefile.dep`; then verify both `make` and `make amalg`.

## Suggested first tests

- Existing Lua: `luaL_loadbufferx(..., mode="t")`, `loadfile("x.lua")`, `luajit -e`, stdin, and REPL still behave exactly as before.
- Bytecode security: bytecode after BOM/shebang is still rejected.
- Bytecode modes: existing `b`, `t`, `bt`, `W`, `X`, `s`, and `d` flows still work through `load`, `loadfile`, `string.dump`, and `luajit -b`.
- Teal file: `luajit sample.tl` compiles and runs without affecting `sample.lua`.
- Teal dump: `luajit -b sample.tl sample.ljbc` produces bytecode that reloads through the existing bytecode reader.
- Amalgamation: `make clean && make` and `make clean && make amalg` both include the Teal objects exactly once.
