# GW-BASIC C Port - Implementation Progress Tracking

**Last Updated:** 2026-08-03
**Based on:** PLAN (`.hermes/plans/2025-08-01_GW-BASIC-C-Port-Implementation.md`) vs Actual Implementation

---

## Phase 1: Core Infrastructure (Weeks 1-2)

| Order | Module | ASM File | PLAN Status | Actual Status | Notes |
|-------|--------|----------|-------------|---------------|-------|
| 1 | `gwdata` | GWDATA.ASM | ✅ Implement | ✅ **DONE** | Variable table, string heap, program buffer |
| 2 | `gwram` | GWRAM.ASM | ✅ Implement | ✅ **DONE** | malloc-based alloc/free/realloc/calloc |
| 3 | `gwinit` | GWINIT.ASM | ✅ Implement | ✅ **DONE** | PI constant, math module init |
| 4 | `gwmain` | GWMAIN.ASM | ✅ Implement | ✅ **DONE** | REPL loop, line editing, command dispatch |
| 5 | `gwsts` | GWSTS.ASM | ✅ Implement | ✅ **DONE** | Statement engine (DIM, LET, PRINT, IF, OPEN, CLOSE, LINE INPUT, GET, PUT, assignment) |
| 6 | `gweval` | GWEVAL.ASM | ✅ Implement | ✅ **DONE** | Expression parser, math functions (11 funcs), variables, arrays |
| 7 | `bimisc` | BIMISC.ASM | ✅ Implement | ✅ **DONE** | NEW, CLEAR, RUN setup, RESTORE, STOP/END/CONT, TRON/TROFF, SWAP, ERASE, trap handling, stack mgmt |
| 8 | `gengrp` | GENGRP.ASM | ✅ Implement | ✅ **DONE** | LINE command, PSET/PRESET/POINT, coordinate scanning, graphics init |

---

## Phase 2: I/O Subsystem (Weeks 2-3)

| Order | Module | ASM File | PLAN Status | Actual Status | Notes |
|-------|--------|----------|-------------|---------------|-------|
| 9 | `giocon` | GIOCON.ASM | ✅ Implement | ✅ **DONE** | Console I/O (PRINT, LOCATE, COLOR, CLS) - Windows CONIO |
| 10 | `giokyb` | GIOKYB.ASM | ✅ Implement | ✅ **DONE** | INKEY$, INPUT$ |
|| 11 | `gioscn` | GIOSCN.ASM | ✅ Implement | ✅ **DONE** | SCREEN modes, PSET, PRESET, POINT - framebuffer simulated, exported globals for gengrp |
| 12 | `scndrv` | SCNDRV.ASM | ✅ Implement | ✅ **DONE** | Text screen driver - cursor, wrapping, scrolling, control chars |
| 13 | `giolpt` | GIOLPT.ASM | ⚠️ Stub | ✅ **DONE** | Printer output - LPT1/2/3, LPOS, WIDTH, CR/LF translation |
| 14 | `giocom` | GIOCOM.ASM | ⚠️ Stub | ✅ **DONE** | Serial comms - COM1..4, baud/parity/data/stop, async I/O, timeout/CTL-Z fixes |
| 15 | `giodsk` | GIODSK.ASM | ✅ Implement | ✅ **DONE** | Disk I/O entry points - thin wrappers around DSKCOM |

---

## Phase 3: Language Features (Weeks 3-4)

| Order | Module | ASM File | PLAN Status | Actual Status | Notes |
|-------|--------|----------|-------------|---------------|-------|
| 16 | `gwlist` | GWLIST.ASM | ✅ Implement | ✅ **DONE** | Program line storage, LIST, add/delete lines |
| 17 | `bistrs` | BISTRS.ASM | ✅ Implement | ✅ **DONE** | LEFT$, RIGHT$, MID$, LEN, ASC, CHR$, STR$, VAL, STRING$, SPACE$ |
| 18 | `biprtu` | BIPRTU.ASM | ✅ Implement | ✅ **DONE** | PRINT USING (basic numeric/string) |
| 19 | `biptrg` | BIPTRG.ASM | ✅ Implement | ✅ **DONE** | DIM arrays, get/set numeric & string elements |
| 20 | `math1` | MATH1.ASM | ✅ Implement | ✅ **DONE** | SIN, COS, TAN, ATN, EXP, LOG |
| 21 | `math2` | MATH2.ASM | ✅ Implement | ✅ **DONE** | SQR, ABS, SGN, RND, INT, FIX |
| 22 | `fiveo` | FIVEO.ASM | ✅ Implement | ❌ **STUB ONLY** | Merged into dskcom |
| 23 | `dskcom` | DSKCOM.ASM | ✅ Implement | ✅ **DONE** | OPEN/CLOSE/PRINT#/INPUT#/LINE INPUT#/GET/PUT/SEEK |

---

## Phase 4: Advanced/Platform (Weeks 4-5)

| Order | Module | ASM File | PLAN Status | Actual Status | Notes |
|-------|--------|----------|-------------|---------------|-------|
| 24 | `oem` | OEM.ASM | ✅ Implement | ❌ **STUB ONLY** | OEM abstraction layer |
| 25 | `oemcbk` | OEMCBK.ASM | ✅ Implement | ❌ **STUB ONLY** | OEM callback handlers |
| 26 | `oemev` | OEMEV.ASM | ✅ Implement | ❌ **STUB ONLY** | Event handling (ON KEY, ON STRIG) |
| 27 | `oemsnd` | OEMSND.ASM | ⚠️ Stub | ❌ **STUB ONLY** | Sound (SOUND, PLAY) |
| 28 | `ibmres` | IBMRES.ASM | ⚠️ Stub | ❌ **STUB ONLY** | IBM PC resource handling |
| 29 | `advgrp` | ADVGRP.ASM | ✅ Implement | ❌ **STUB ONLY** | Advanced statements (DEF FN, ERASE) |
| 30 | `call86` | CALL86.ASM | ✅ Implement | ❌ **STUB ONLY** | CALL absolute, USR functions |
| 31 | `itsa86` | ITSA86.ASM | ✅ Implement | ❌ **STUB ONLY** | Instruction analysis |
| 32 | `next86` | NEXT86.ASM | ✅ Implement | ❌ **STUB ONLY** | NEXT loop handling |
| 33 | `kanj86` | KANJ86.ASM | ⚠️ Stub | ❌ **STUB ONLY** | Kanji/Japanese support |
| 34 | `maclng` | MACLNG.ASM | ✅ Implement | ❌ **STUB ONLY** | Machine language interface |
| 35 | `scnedt` | SCNEDT.ASM | ✅ Implement | ❌ **STUB ONLY** | Screen editor (line editing) |
| 36 | `giocas` | GIOCAS.ASM | ⚠️ Stub | ❌ **STUB ONLY** | Cassette I/O |
| 37 | `gio86` | GIO86.ASM | ⚠️ Stub | ❌ **STUB ONLY** | 8086-specific I/O |
| 38 | `giotbl` | GIOTBL.ASM | ⚠️ Stub | ❌ **STUB ONLY** | I/O tables |
| 39 | `biboot` | BIBOOT.ASM | ✅ Implement | ❌ **STUB ONLY** | Boot/startup |

---

## Summary Statistics

| Category | Count | Percentage |
|----------|-------|------------|
| **Fully Implemented** | 22 | 56% |
| **Partially Implemented** | 0 | 0% |
| **Stub Only (TODO)** | 17 | 44% |
| **Total Modules** | 39 | 100% |

---

## Test Coverage

| Test File | Module(s) Covered | Status |
|-----------|-------------------|--------|
| test_gwdata.c | gwdata | ✅ Passing |
| test_gwram.c | gwram | ✅ Passing |
| test_gwinit.c | gwinit | ✅ Passing |
| test_gwsts_gweval.c | gwsts, gweval | ✅ Passing |
| test_giocon.c | giocon | ✅ Passing |
| test_giokyb_gioscn.c | giokyb, gioscn | ✅ Passing |
| test_gwlist_bistrs.c | gwlist, bistrs | ✅ Passing |
| test_biprtu_biptrg.c | biprtu, biptrg | ✅ Passing |
| test_dskcom.c | dskcom | ✅ Passing |
| test_math.c | math1, math2 | ✅ Passing |

**Total Tests:** 206 passing, 0 failing

---

## Next Module to Implement (per PLAN order)

**Phase 4 modules (24-39) - 18 modules remaining:**

| Order | Module | ASM File | PLAN Status | Notes |
|-------|--------|----------|-------------|-------|
| 24 | `oem` | OEM.ASM | ❌ **STUB ONLY** | OEM abstraction layer |
| 25 | `oemcbk` | OEMCBK.ASM | ❌ **STUB ONLY** | OEM callback handlers |
| 26 | `oemev` | OEMEV.ASM | ❌ **STUB ONLY** | Event handling (ON KEY, ON STRIG) |
| 27 | `oemsnd` | OEMSND.ASM | ❌ **STUB ONLY** | Sound (SOUND, PLAY) |
| 28 | `ibmres` | IBMRES.ASM | ❌ **STUB ONLY** | IBM PC resource handling |
| 29 | `advgrp` | ADVGRP.ASM | ❌ **STUB ONLY** | Advanced statements (DEF FN, ERASE) |
| 30 | `call86` | CALL86.ASM | ❌ **STUB ONLY** | CALL absolute, USR functions |
| 31 | `itsa86` | ITSA86.ASM | ❌ **STUB ONLY** | Instruction analysis |
| 32 | `next86` | NEXT86.ASM | ❌ **STUB ONLY** | NEXT loop handling |
| 33 | `kanj86` | KANJ86.ASM | ❌ **STUB ONLY** | Kanji/Japanese support |
| 34 | `maclng` | MACLNG.ASM | ❌ **STUB ONLY** | Machine language interface |
| 35 | `scnedt` | SCNEDT.ASM | ❌ **STUB ONLY** | Screen editor (line editing) |
| 36 | `giocas` | GIOCAS.ASM | ❌ **STUB ONLY** | Cassette I/O |
| 37 | `gio86` | GIO86.ASM | ❌ **STUB ONLY** | 8086-specific I/O |
| 38 | `giotbl` | GIOTBL.ASM | ❌ **STUB ONLY** | I/O tables |
| 39 | `biboot` | BIBOOT.ASM | ❌ **STUB ONLY** | Boot/startup |

**Priority recommendation:**
1. **advgrp** - Advanced statements (DEF FN, ERASE)
2. **gioscn framebuffer** - Complete graphics support
3. **scnedt** - Screen editor/line editing
4. **call86** - CALL absolute, USR functions