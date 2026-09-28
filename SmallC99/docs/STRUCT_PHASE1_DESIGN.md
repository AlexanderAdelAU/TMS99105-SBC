# SMALLC99 — Structures, Phase 1

## As built

Phase 1 is complete and tested: `STRUCTTEST.C` passes 60/60, `LANGTEST`
passes 133/133, and 23 existing programs (LANGTEST, the fixtures and the
compiler's own sources) compile to byte-identical assembly. The design
below was followed, with these changes found during the work:

| Planned | As built | Why |
|---|---|---|
| Struct parser in the DECL overlay | Its own overlay, **ID 8 `STRD`, physical page 15**, segment 8 (`CC_STRD.C`) | Compiled, it came to ~2.4K and overflowed DECL's page |
| DECL calls the parser directly | DECL, STMT and DFUN all call it through the framed trampoline **`R_STRUCT`** (`OVLSTUBS.A99`) | It is a different overlay |
| 32 tags, 64 members | **16 tags, 40 members** (1,240 bytes) | The space above the resident part is the C run-time heap; an `#include` needs ~1,700 bytes of it and is silently dropped below that |
| `elsize()` etc. resident, in C | **`elsize()` resident in assembly** (`CC_STRU.A99`, 46 bytes); `findtag()`/`findmemb()` in the expression overlay | The C versions cost ~450 resident bytes |
| 13 `type >> 2` sites | **14** — the resident `decl()` also had one | |
| `.`/`->` code in EXPR_B | `member()` in EXPR_A, scaling in EXPR_C, lookups in EXPR_B | Page space; the four expression pages are one overlay, so calls are unaffected |

**Resident cost:** the heap drops from 3,472 to 1,960 bytes (the tables, plus
`elsize()`; `R_STRUCT` fits in the linker's block padding).

**Overlay budgets** (chain-safe limit 4,032): EXPR_A 3,820, EXPR_C 3,724,
EXPR_B 3,664, EXPR_D 3,372, STMT 3,788, DECL 2,706, STRD 2,454, DFUN 2,082.

**Toolchain:** SMALLC99's total code now passes 64K, which link99 3.9.56
cannot link correctly. **link99 3.9.58 is required** (see its history):
code offsets past 64K, relocation entries widened to 6 bytes, the XRPLUS
marker compare, and `ifilelbuf()` in `seek()`. `make_smallc99.ps1` now
insists on 3.9.58.

**Errors are silent:** SMALLC99's `error()` discards its message, so a
struct mistake (an unknown member, a struct passed by value) produces no
diagnostic on the SBC.

---

## Original design note

## Scope

**In:** `struct` and `union` declarations — global, local and as pointer
parameters; the `.` and `->` operators; arrays of structs; pointers to structs
(including `p++`, `p + n`, `p[i]`, `p - q`); structs nested inside structs;
self-referencing structs (`struct node { struct node *next; }`);
`sizeof(struct tag)` and `sizeof(variable)`.

**Out (later phases):** struct assignment (`a = b`), passing or returning a
struct by value, struct initialisers, defining one struct inside another's
body, bit-fields, `typedef`.

Any program that does not use `struct` or `union` must compile to exactly the
same assembly as today. This is the first and strongest test.

## 1. Type encoding — no change to the symbol-table layout

Today a type byte is `size << 2 | unsigned`: `CHR` = 4, `UCHR` = 5,
`INT` = 8, `UINT` = 9. Bit 1 is never set.

A struct type uses that spare bit:

```
struct type  =  (tag index << 2) | 2        STRUCTBIT = 2
```

- Up to 64 tags. The tag index points into a new tag table.
- The type travels through the expression engine unchanged, in `is[TI]` and
  `is[TA]`, exactly as `CHR` and `INT` do — so pointer arithmetic on a struct
  pointer knows which struct it points to.
- Symbol entries keep their current layout (IDENT, TYPE, CLASS, SIZE, OFFSET,
  NAME). Nothing moves in the 13 modules' private `#define`s.

The cost: 13 places use `type >> 2` to mean "element size". For a struct type
that would give the tag index instead. Each is changed to call a new resident
function `elsize(type)`, which returns 1, 2, or the struct's size:

| Module | Line | Use | Change |
|--------|------|-----|--------|
| EXPR_A | 298, 305 | is it a char (size 1)? | a struct is never size 1 |
| EXPR_B | 234, 241 | subscript scaling | `elsize()` + general scaling |
| EXPR_B | 303, 305 | `++`/`--` step size | `elsize()` |
| EXPR_B | 313 | does pointer arithmetic need doubling? | general scaling |
| EXPR_C | 251–252 | pointer difference | divide by `elsize()` |
| EXPR_D | 176 | local scalar char `+1` byte adjust | exclude structs |
| EXPR_D | 302, 310 | fetch a word or a byte | refuse a whole struct |
| DECL | 128, 130, 135 | object size | `elsize()` |

## 2. Tables (resident, in CC_DATA)

**Tag table** — 32 entries × 20 bytes = 640 bytes:
name (16), size (2), first member (1), member count (1).

**Member table** — 64 entries × 23 bytes = 1,472 bytes, in the same record
format as a global symbol: IDENT, TYPE, CLASS (= owning tag), SIZE,
OFFSET (= byte offset within the struct), NAME.

Members of one struct are stored contiguously, so looking up `s.x` scans only
that struct's members. Different structs may reuse member names.

Total 2,112 bytes of the ~3,400 free below `>8000`. Both limits are `#define`s.

**Layout rules** (TMS9900: words must be at even addresses):
- `char` members take one byte at any offset.
- `int`, pointers and struct members are placed at an even offset.
- A struct's size is rounded up to even, so arrays of structs stay aligned.
- A union's members all have offset 0; its size is the largest member, rounded
  to even.

## 3. Declarations

A new function `dostruct()` parses `struct|union tag` with an optional
`{ member-list }` and returns the struct type code:

- A tag is registered **before** its body is parsed, so
  `struct node { struct node *next; }` works: a pointer member needs only the
  pointer size, and the struct's own size is looked up later when it is used.
- `struct tag;` or a use of `struct tag` without a body refers to an existing
  tag, or creates an incomplete one that a later body completes.

`dostruct()` lives in the **DECL** overlay (1,836 bytes free). DECL uses it
directly for globals. STMT (local declarations) and DFUN (parameters) reach it
through one new framed trampoline, `R_STRUCT`, in the resident part — the same
mechanism as `R_STMT` and `R_TEST`.

| Where | Change |
|-------|--------|
| DECL `dodeclare`/`declglb` | accept `struct`/`union`; object sizes via `elsize()` |
| STMT `declloc` | accept `struct`/`union` locals; stack space rounded to even |
| DFUN parameter declarations | accept `struct tag *p` |

A struct variable's SIZE field holds its full size (`dim × struct size` for an
array), so the existing `sizeof(variable)` code works unchanged.

## 4. Expressions

`.` and `->` join `[` and `(` in `level14` (EXPR_B, 962 bytes free):

- **`s.m`** — get the struct's address into the primary register
  (a global struct: `POINT1m`; a local, an array element or another member:
  already there), add the member's offset, then make the result an lvalue of
  the member's type — exactly the state a subscript leaves behind, so fetch,
  store, `&`, `++` and further `[` / `.` / `->` all work unchanged.
- **`p->m`** — fetch the pointer's value, then as `.`.
- Member lookup is a small resident function `findmemb(tag, name)`, so the
  expression overlay calls it directly.

**Scaling by a struct size** needs no new p-codes and no change to the code
generator:

| Case | Code |
|------|------|
| constant index or offset | `index × size` worked out at compile time |
| variable index | `SWAP12, PUSH1, SWAP12, GETw2n size, MUL12, POP2` |
| `p - q` of struct pointers | `SWAP12, GETw1n size, DIV12` |
| `p++`, `p--` | step by `elsize()` |

Using a whole struct as a value (`x = s`, `f(s)`) is reported as an error rather
than silently fetching one word.

`sizeof` gains `sizeof(struct tag)`.

## 5. Space plan

| Module | Free now | Adds |
|--------|----------|------|
| Resident | ~3,400 | tables 2,112; `elsize`, `findmemb`, `R_STRUCT` ~250 |
| DECL | 1,836 | `dostruct` + member parsing ~900 |
| STMT | 466 | `struct` in `declloc` ~150 |
| DFUN | 2,268 | struct pointer parameters ~100 |
| EXPR_B | 962 | `.`, `->`, scaling ~600 |
| EXPR_C | 818 | pointer difference ~80 |
| EXPR_A, EXPR_D | 1,612, 892 | guards and `sizeof` ~150 |

If a page runs short, functions move to a page with room — the four expression
pages are one overlay, so no calls change.

## 6. Testing

1. **No regressions.** Programs without structs — the compiler's own sources
   and `LANGTEST_LANGUAGE.C` — must produce byte-identical assembly before and
   after.
2. **Run the real compiler here.** The current `SMALLC99.EXE` runs in the
   TMS9900 emulator with the overlay mapper and a BDOS file mock, so every
   change is tested on the actual binary, not a host imitation.
3. **A new `STRUCTTEST.C`** in LANGTEST style (`expect()` numbered checks):
   member access, nesting, arrays, pointer arithmetic, unions, `sizeof`, and
   the self-referencing list case — run in the emulator, then on the SBC.
4. **The build's own checks:** drel chain lint and the TRSTACK depth limit.
