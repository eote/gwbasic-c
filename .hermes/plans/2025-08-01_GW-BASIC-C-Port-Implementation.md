# GW-BASIC C Port Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Implement a fully functional GW-BASIC interpreter in C, following the original 1983 Microsoft GW-BASIC assembly source code structure (one C file per ASM module), maintaining 1:1 module correspondence.

**Architecture:** 
- 39 C modules mirroring the original ASM files, each with `*_init()` function called from `gwmain.c`
- Single global runtime state (`g_state`) holding interpreter context
- Console-based I/O using standard C library (stdio, conio.h for Windows)
- Modular design allowing incremental implementation and testing

**Tech Stack:**
- C11 (GCC/MinGW-w64 on Windows)
- Make-based build system
- No external dependencies beyond standard library

---

## Current Context

**Completed (Skeleton):**
- ✅ `Makefile` — builds all 39 modules + gwmain
- ✅ `src/gwbasic.h` — common types, forward declarations
- ✅ `src/gwmain.c` — entry point, module initialization chain, stub main loop
- ✅ 39 module stubs (`advgrp.c` through `scnedt.c`) with empty `*_init()`
- ✅ Clean build + executable runs

**Source Reference:** `/d/myplace/workspace/projects/gwbasic/GW-BASIC/*.ASM` (39 files)

---

## Module Dependency Order (Implementation Sequence)

Based on GW-BASIC architecture, implement in this order:

### Phase 1: Core Infrastructure (Weeks 1-2)
| Order | Module | Original ASM | Responsibility |
|-------|--------|--------------|----------------|
| 1 | `gwdata` | GWDATA.ASM | Global data structures, variable tables, string heap |
| 2 | `gwram` | GWRAM.ASM | Memory management, allocation/free |
| 3 | `gwinit` | GWINIT.ASM | Interpreter initialization, default state |
| 4 | `gwmain` | GWMAIN.ASM | Main loop, command dispatch (already has skeleton) |
| 5 | `gwsts` | GWSTS.ASM | Statement execution engine |
| 6 | `gweval` | GWEVAL.ASM | Expression evaluation |
| 7 | `bimisc` | BIMISC.ASM | Miscellaneous built-in functions |
| 8 | `gengrp` | GENGRP.ASM | General statement group handlers |

### Phase 2: I/O Subsystem (Weeks 2-3)
| Order | Module | Original ASM | Responsibility |
|-------|--------|--------------|----------------|
| 9 | `giocon` | GIOCON.ASM | Console I/O (PRINT, INPUT, LOCATE, COLOR) |
| 10 | `giokyb` | GIOKYB.ASM | Keyboard input (INKEY$, INPUT$) |
| 11 | `gioscn` | GIOSCN.ASM | Screen handling (CLS, SCREEN, graphics modes) |
| 12 | `scndrv` | SCNDRV.ASM | Screen driver abstraction |
| 13 | `giolpt` | GIOLPT.ASM | Printer output (LPRINT, LLIST) — **stub for now** |
| 14 | `giocom` | GIOCOM.ASM | Serial communications (OPEN COM) — **stub for now** |
| 15 | `giodsk` | GIODSK.ASM | Disk I/O (OPEN, GET, PUT, LOAD, SAVE) — merged into dskcom |

### Phase 3: Language Features (Weeks 3-4)
| Order | Module | Original ASM | Responsibility |
|-------|--------|--------------|----------------|
| 16 | `gwlist` | GWLIST.ASM | LIST, LLIST, program storage |
| 17 | `bistrs` | BISTRS.ASM | String functions (MID$, LEFT$, RIGHT$, etc.) |
| 18 | `biprtu` | BIPRTU.ASM | PRINT USING formatting |
| 19 | `biptrg` | BIPTRG.ASM | Pointer/array handling |
| 20 | `math1` | MATH1.ASM | Basic math (ABS, SGN, SQR, RND) |
| 21 | `math2` | MATH2.ASM | Transcendental (SIN, COS, TAN, ATN, LOG, EXP) |
| 22 | `fiveo` | FIVEO.ASM | File I/O statements (OPEN, CLOSE, etc.) — merged into dskcom |
| 23 | `dskcom` | DSKCOM.ASM | Disk command handling |

### Phase 4: Advanced/Platform (Weeks 4-5)
| Order | Module | Original ASM | Responsibility |
|-------|--------|--------------|----------------|
| 24 | `oem` | OEM.ASM | OEM abstraction layer |
| 25 | `oemcbk` | OEMCBK.ASM | OEM callback handlers |
| 26 | `oemev` | OEMEV.ASM | Event handling (ON KEY, ON STRIG, etc.) |
| 27 | `oemsnd` | OEMSND.ASM | Sound (SOUND, PLAY) — **stub for now** |
| 28 | `ibmres` | IBMRES.ASM | IBM PC resource handling — **stub for now** |
| 29 | `advgrp` | ADVGRP.ASM | Advanced statements (DEF FN, ERASE, etc.) |
| 30 | `call86` | CALL86.ASM | CALL absolute, USR functions |
| 31 | `itsa86` | ITSA86.ASM | Instruction analysis |
| 32 | `next86` | NEXT86.ASM | NEXT loop handling |
| 33 | `kanj86` | KANJ86.ASM | Kanji/Japanese support — **stub for now** |
| 34 | `maclng` | MACLNG.ASM | Machine language interface |
| 35 | `scnedt` | SCNEDT.ASM | Screen editor (line editing) |
| 36 | `giocas` | GIOCAS.ASM | Cassette I/O (legacy) — **stub for now** |
| 37 | `gio86` | GIO86.ASM | 8086-specific I/O — **stub for now** |
| 38 | `giotbl` | GIOTBL.ASM | I/O tables — **stub for now** |
| 39 | `biboot` | BIBOOT.ASM | Boot/startup |

**Note:** Modules marked "stub for now" will have minimal `*_init()` implementations to satisfy the 1:1 module structure. Full implementation deferred to later phases when modern OS adaptations are designed.

---

## Detailed Tasks (Bite-Sized, TDD)

### Task 1: Add Unit Test Framework

**Objective:** Set up minimal test infrastructure for module-level testing

**Files:**
- Create: `tests/test_framework.h` — simple assert macros
- Create: `tests/Makefile` — builds test runner
- Modify: `Makefile` — add `test` target

**Step 1: Write test framework header**
```c
/* tests/test_framework.h */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, msg); \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ_INT(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "FAIL [%s:%d] %s: got %d, expected %d\n", \
                    __FILE__, __LINE__, msg, (actual), (expected)); \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_EQ_STR(actual, expected, msg) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            fprintf(stderr, "FAIL [%s:%d] %s: got \"%s\", expected \"%s\"\n", \
                    __FILE__, __LINE__, msg, (actual), (expected)); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS(msg) \
    do { \
        printf("PASS: %s\n", msg); \
        return 0; \
    } while (0)

typedef int (*test_fn_t)(void);

int run_test(const char *name, test_fn_t fn);

#endif
```

**Step 2: Write test runner**
```c
/* tests/test_runner.c */
#include "test_framework.h"

int run_test(const char *name, test_fn_t fn) {
    printf("RUN: %s... ", name);
    fflush(stdout);
    int result = fn();
    if (result == 0) printf("OK\n");
    return result;
}
```

**Step 3: Add to Makefile**
```makefile
# Add to main Makefile
TEST_SRCS = tests/test_runner.c tests/test_gwdata.c tests/test_gwram.c ...
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_TARGET = test_gwbasic.exe

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(filter-out src/gwmain.o, $(OBJS)) $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
```

**Verification:** `make test` compiles and runs (0 tests initially)

---

### Task 2: Implement GWDATA (Global Data Structures)

**Objective:** Define all core data structures: variable table, string heap, program buffer, runtime state

**Files:**
- Modify: `src/gwdata.c` — implement data structures
- Modify: `src/gwbasic.h` — add public type definitions
- Create: `tests/test_gwdata.c` — unit tests

**Step 1: Write failing test**
```c
/* tests/test_gwdata.c */
#include "test_framework.h"
#include "../src/gwbasic.h"
#include "../src/gwdata.c"  /* include for static access */

int test_gwdata_variable_table(void) {
    gwdata_init();
    /* Test variable allocation */
    struct variable *v = gwdata_alloc_variable("A", VT_SINGLE);
    TEST_ASSERT(v != NULL, "variable allocation");
    TEST_ASSERT_EQ_STR(v->name, "A", "variable name");
    TEST_ASSERT_EQ_INT(v->type, VT_SINGLE, "variable type");
    TEST_PASS("variable table basic ops");
}

int test_gwdata_string_heap(void) {
    gwdata_init();
    char *s = gwdata_alloc_string(10);
    TEST_ASSERT(s != NULL, "string allocation");
    strcpy(s, "HELLO");
    TEST_ASSERT_EQ_STR(s, "HELLO", "string content");
    gwdata_free_string(s);
    TEST_PASS("string heap basic ops");
}

int main(void) {
    int failed = 0;
    failed += run_test("gwdata_variable_table", test_gwdata_variable_table);
    failed += run_test("gwdata_string_heap", test_gwdata_string_heap);
    return failed ? 1 : 0;
}
```

**Step 2: Run test → verify FAIL**

**Step 3: Implement gwdata.c**
```c
/* src/gwdata.c — key structures */
#include "gwbasic.h"
#include <string.h>

#define MAX_VARIABLES  256
#define STRING_HEAP_SIZE  16384
#define PROGRAM_BUFFER_SIZE  32768

enum var_type { VT_INTEGER=2, VT_SINGLE=4, VT_DOUBLE=8, VT_STRING=3 };

struct variable {
    char name[MAX_VAR_NAME];
    enum var_type type;
    union {
        int32_t  i;
        float    f;
        double   d;
        char    *s;
    } value;
    struct variable *next;
};

struct string_heap {
    char data[STRING_HEAP_SIZE];
    size_t used;
};

struct program_buffer {
    char data[PROGRAM_BUFFER_SIZE];
    size_t used;
};

struct _runtime_state {
    bool running;
    int exit_code;
    struct variable *var_table[MAX_VARIABLES];
    struct string_heap str_heap;
    struct program_buffer prog_buf;
    /* interpreter state */
    uint16_t current_line;
    bool in_direct_mode;
};

struct _runtime_state *g_state = NULL;

void gwdata_init(void) {
    if (g_state) return;
    g_state = calloc(1, sizeof(*g_state));
    g_state->running = true;
    g_state->in_direct_mode = true;
    g_state->str_heap.used = 0;
    g_state->prog_buf.used = 0;
}

struct variable *gwdata_alloc_variable(const char *name, enum var_type type) {
    /* hash name, allocate, link into table */
}
char *gwdata_alloc_string(size_t len) { /* bump alloc from heap */ }
void gwdata_free_string(char *s) { /* mark free, coalesce */ }
```

**Step 4: Run test → verify PASS**

**Step 5: Commit**
```bash
git add src/gwbasic.h src/gwdata.c tests/test_gwdata.c
git commit -m "feat(gwdata): implement variable table, string heap, program buffer"
```

---

### Task 3: Implement GWRAM (Memory Management)

**Objective:** Provide alloc/free for interpreter objects (arrays, file buffers, etc.)

**Files:**
- Modify: `src/gwram.c`
- Create: `tests/test_gwram.c`

**Test:** Allocate/free blocks, verify coalescing, OOM handling

---

### Task 4: Implement GWINIT (Interpreter Initialization)

**Objective:** Set up default state, built-in constants, function tables

**Files:**
- Modify: `src/gwinit.c`
- Create: `tests/test_gwinit.c`

**Test:** Verify default variables (PI, etc.), function pointers populated

---

### Task 5: Implement GWSTS (Statement Engine) — Core Dispatch

**Objective:** Parse and execute basic statements: LET, PRINT, IF, GOTO, GOSUB, RETURN, FOR, NEXT, END, STOP

**Files:**
- Modify: `src/gwsts.c`
- Create: `tests/test_gwsts.c`

**Approach:** Statement table with handler functions; start with minimal subset

---

### Task 6: Implement GWEVAL (Expression Evaluation)

**Objective:** Evaluate arithmetic, relational, logical expressions with operator precedence

**Files:**
- Modify: `src/gweval.c`
- Create: `tests/test_gweval.c`

**Test:** `3+4*2 == 11`, `"A"+"B" == "AB"`, `NOT (1>2) == -1`

---

### Task 7: Implement GIOCON (Console I/O)

**Objective:** PRINT, INPUT, LOCATE, COLOR, CLS (text mode)

**Files:**
- Modify: `src/giocon.c`
- Create: `tests/test_giocon.c`

**Note:** Use ANSI escape codes or Windows CONIO for cursor/color

---

### Task 8: Implement GWLIST (Program Storage & Listing)

**Objective:** Store program lines in memory, LIST command, line editing

**Files:**
- Modify: `src/gwlist.c`
- Create: `tests/test_gwlist.c`

---

### Task 9: Implement GWMAIN Main Loop

**Objective:** Replace stub with real REPL: prompt → read line → parse → execute → repeat

**Files:**
- Modify: `src/gwmain.c`

**Test:** Manual — run `./gwbasic.exe`, type `PRINT 2+2`, see `4`

---

### Task 10: Implement Math Functions (MATH1, MATH2)

**Objective:** ABS, SGN, SQR, RND, SIN, COS, TAN, ATN, LOG, EXP

**Files:**
- Modify: `src/math1.c`, `src/math2.c`
- Create: `tests/test_math.c`

---

### Task 11: Implement String Functions (BISTRS)

**Objective:** MID$, LEFT$, RIGHT$, LEN, ASC, CHR$, STR$, VAL, STRING$, SPACE$

**Files:**
- Modify: `src/bistrs.c`
- Create: `tests/test_bistrs.c`

---

### Task 12: Implement File I/O (GIODSK, FIVEO, DSKCOM)

**Objective:** OPEN, CLOSE, GET, PUT, INPUT#, PRINT#, LOAD, SAVE, KILL, NAME

**Files:**
- Modify: `src/giodsk.c`, `src/fiveo.c`, `src/dskcom.c`
- Create: `tests/test_fileio.c`

---

### Task 13: Implement Graphics (GIOSCN, SCNDRV)

**Objective:** SCREEN modes, PSET, PRESET, LINE, CIRCLE, PAINT, DRAW

**Files:**
- Modify: `src/gioscn.c`, `src/scndrv.c`
- Create: `tests/test_graphics.c`

---

### Task 14: Implement Remaining Modules

**Objective:** Complete all 39 modules per dependency order

**Files:** Each module + test

**Priority modules for BASIC compatibility:**
- `giokyb` (INKEY$, INPUT$)
- `biprtu` (PRINT USING)
- `oem`/`oemcbk`/`oemev`/`oemsnd` (events, sound)
- `call86`, `itsa86`, `next86` (CALL, USR, loops)
- `scnedt` (line editor)

---

## Testing Strategy

| Level | Tool | Scope |
|-------|------|-------|
| Unit | Custom test runner | Each module in isolation |
| Integration | `make test` | All modules linked |
| Functional | Shell scripts | Run GW-BASIC programs, compare output |
| Regression | Test suite | `tests/basics/*.bas` → expected output |

**Functional Test Example:**
```bash
# tests/functional/test_print.bas
PRINT "HELLO WORLD"
PRINT 2+3*4
END
```
```bash
# Run and verify
./gwbasic.exe < tests/functional/test_print.bas > /tmp/out.txt
diff /tmp/out.txt tests/functional/test_print.expected
```

---

## Risks & Tradeoffs

| Risk | Mitigation |
|------|------------|
| 16-bit memory model assumptions | Use explicit-sized types (`uint16_t`, `int32_t`), avoid pointer arithmetic dependent on 16-bit segments |
| Assembly-specific optimizations (self-modifying code, etc.) | Re-implement logic in portable C; document deviations |
| Hardware dependencies (video, sound, serial) | Abstract behind OEM layer; stub unsupported features |
| Floating-point differences (MBF vs IEEE) | MBF format used in GW-BASIC — implement MBF<->IEEE conversion for math functions |
| Line-numbered program editing | Implement linked-list line buffer with renumber support |

---

## Open Questions

1. **MBF Floating Point:** Original uses Microsoft Binary Format (4/8 bytes). Implement custom MBF math or convert to IEEE at boundaries?
2. **Screen Modes:** Full graphics emulation (CGA/EGA) vs text-mode only for v1?
3. **Tokenization:** Original uses tokenized program storage. Store as text (simpler) or tokens (authentic)?
4. **Keyboard/Event Model:** Polling vs callback? Original uses interrupt-driven.

---

## Success Criteria

- [ ] All 39 modules implemented with passing unit tests
- [ ] `make test` passes (100% unit test coverage for core modules)
- [ ] Runs Microsoft's GW-BASIC test suite (if available)
- [ ] Executes classic BASIC programs: NIM, HANGMAN, STAR TREK
- [ ] `PRINT 2+2` → `4` in interactive mode
- [ ] `LOAD`/`SAVE` works with tokenized `.BAS` files

---

*Plan generated: 2025-08-01. Update as implementation progresses.*