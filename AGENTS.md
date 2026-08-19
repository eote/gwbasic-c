# GW-BASIC C Port — AGENTS.md

A 1:1 C port of the Microsoft GW-BASIC 1983 ASM source (39 modules). All 39 modules implemented, 1084 unit tests passing on Windows/MinGW64.

## Dev environment
- OS: Windows 10/11 with MinGW-w64 (gcc)
- Build tool: `mingw32-make` (not `make` — missing from PATH)
- Compiler: `gcc -std=c11 -Wall -Wextra -O2`
- Target: `gwbasic.exe` (PE32+ console)

## Build & test
```bash
# Full rebuild + test (reviewer's canonical command)
mingw32-make clean && mingw32-make test

# Debug build
mingw32-make debug

# Run single .bas test
cmd /c "D:\myplace\workspace\projects\gwbasic\c\gwbasic.exe D:\myplace\workspace\projects\gwbasic\c\tests\bas\<file>.bas"
```

## Project layout
```
src/           # 39 .c modules + gwbasic.h (all structs defined here)
tests/         # 26 test_*.c files + test_runner.c + test_framework.h
tests/bas/     # 26 .bas integration tests
.d/            # Build artifacts (.o files)
```

## Conventions (observed from code)
- **Pointer-to-int**: `intptr_t` everywhere — never `(int)` cast
- **String compare**: ASCII-only `strcasecmp_ascii()` — no POSIX `strcasecmp`
- **Debug**: `DEBUG_PRINT(fmt, ...)` macro, enabled via `-DDEBUG` in CFLAGS
- **Structs**: declared only in `gwbasic.h` — never re-declare in `.c`
- **Variable types**: `VT_INTEGER=2`, `VT_SINGLE=4`, `VT_DOUBLE=8`, `VT_STRING=3` (suffixes `%`, `!`, `#`, `$`)
- **Return codes**: `SUCCESS=0`, `ERROR=-1` (windows.h `ERROR` is undefined and redefined)
- **Line length**: max 255 chars (`MAX_LINE_LEN`); variable names max 40 (`MAX_VAR_NAME`)

## Pitfalls
- **Pixel bitmask advance**: check LOW bits (`mask & 0x01` / `mask & 0x03`), not high bits (`0x80`/`0xC0` → infinite loop in one byte)
- **GET/PUT packing**: MSB-first — pixel at mask position stays at mask position in packed byte
- **`gengrp.c` framebuffer stride**: use `MAX_WIDTH` (640), not `g_fb_width`
- **`gengrp_point()`**: must NOT call `scan_coord()`
- **`test_runner.c` registration**: NEVER use `patch replace_all=true` — corrupts the file
- **FOR/NEXT**: FOR pushes variable name + next-line text pointer; NEXT finds matching FOR by name and jumps via text pointer
- **`gwdata_init()` called twice** (once per test, once at startup) — resets variable table & string heap each time

## Git rules
- No `git commit` / `push` / `restore` / `checkout` without explicit user "批准合并"
- Reviewer reads `CODE_REVIEW.md` before approval