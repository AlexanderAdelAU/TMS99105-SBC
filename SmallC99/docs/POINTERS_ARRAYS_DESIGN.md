# SMALLC99 — Pointers to Pointers, Arrays of Pointers, Multi-dimensional Arrays

## Goal

Make SMALLC99 able to compile its own sources, then add multi-dimensional
arrays on the same foundation.

Compiling its own sources today, SMALLC99 stops on exactly three things:

| Source | Construct | Needs |
|---|---|---|
| `SMALLC99.C` 40, `CC_CL99.C` 161 | `char **argv;` (parameter) | pointer to pointer |
| `CC_CG99.C` 108 | `char *code[PCODEMAX];` | array of pointers |
| `CC_CG99T.C` 95 | `extern char *code[];` | extern array of pointers |
| `CC_SCAN_SYM.C` 343 | a 129-character line | SMALLC99 keeps 127 characters and **silently truncates**; here the cut removes a `*/` and the rest of the file becomes a comment |

## Phase A — pointer depth (for self-hosting)

### Type encoding

Today a type byte is `size << 2 | unsigned` for char/int, or
`tag << 2 | STRUCTBIT` for a struct. Only sizes 1 and 2 exist and only 16
tags are allowed, so bits 6–7 are free in both forms. They become the
**pointer depth**: the number of extra pointer levels in an element type.

```
bits 7-6   extra pointer levels (0-3)
bits 5-2   size (1, 2) or struct tag (0-15)
bit  1     STRUCTBIT
bit  0     unsigned
```

| Declaration | IDENT | TYPE |
|---|---|---|
| `char c;` | VARIABLE | CHR |
| `char *p;` | POINTER | CHR (as today) |
| `char **pp;` | POINTER | CHR + 1 level |
| `char *tab[5];` | ARRAY | CHR + 1 level (elements are `char *`) |
| `struct pt **q;` | POINTER | tag + 1 level |

As today, IDENT supplies one level and TYPE describes what that level points
at or contains, so every existing `char *`, `int *` and array keeps exactly
its current encoding. `char ****` (depth 3 in TYPE) is the maximum.

### One rule for "the thing an address points at"

When the expression engine produces an lvalue from an element type `E`
(dereference, subscript):

- `E` has depth > 0: the element is itself a pointer. `is[TI] = UINT` (fetch
  a word, compare unsigned, exactly as a pointer variable is today) and
  `is[TA] = E` minus one level.
- `E` has depth 0: as today. `is[TI] = E`, `is[TA] = 0`.

`E` comes from `is[TA]` (the address type), not from the symbol entry, so
`pp[1][2]`, `**pp` and `*tab[i]` chain correctly. Where a site uses
`ptr[TYPE]` today, `is[TA]` already equals it, so existing programs compile
identically. The corpus proves that.

### Changes

| Where | Change |
|---|---|
| `elsize()` (CC_STRU, resident asm) | depth > 0 -> 2; tag mask `& 15` |
| EXPR_A unary `*` | element type from `is[TA]`, then the rule above |
| EXPR_B subscript | element type and scaling from `is[TA]`, then the rule above |
| EXPR_A `member()`, `findmemb` | `->` needs a struct pointer of depth 0; tag `& 15` |
| DECL `declglb` | each extra `*` adds a level; `*name[n]` and `extern *name[]` are arrays of pointers |
| resident `decl()` (locals, parameters) | the same, and it returns the adjusted type |
| STMT `declloc`, DFUN `doargs` | take the adjusted type from `decl()`; parameter `char *argv[]` is `char **argv` |
| CC_STRD member declarations | the same declarator rules for members |
| `sizeof` | `sizeof(char **)` etc. |
| "is this a struct object" tests | `STRUCTBIT` and depth 0 |
| CC_PREP `inline()` | a line longer than 127 characters reports "line too long" instead of truncating silently |

Pointer arithmetic needs no change: `elsize()` already scales `pp + 1` by 2.

### Not in phase A

Initialiser lists for arrays of pointers (`char *msg[] = { "a", "b" };`),
pointers to functions, and casts. Checked separately if the self-hosting
test shows the compiler's sources use them.

## Phase B — multi-dimensional arrays

`int m[3][4]` is an array of 3 rows of 4 ints. A row is given a type code
like a struct: a tag-table entry marked as a row, holding the row size
(4 × 2 = 8) and its element type (INT), with no members. Then:

- `m` is ARRAY with the row type; `sizeof(m)` = 3 × 8 = 24.
- `m[i]` scales by the row size through `elsize()` (the struct scaling
  code) and yields the row's address with element type INT (not an lvalue:
  a row is an array).
- `m[i][j]` is then an ordinary subscript.
- Three or more dimensions nest the same way (a row of rows).

The struct work already supplies the type-code form, `elsize()` and scaling
by any size, so phase B is mostly declaration parsing plus the "a row is an
address" rule in the subscript code.

## Phase C — self-hosting test

1. SMALLC99 (running in `sbcemu`) compiles all its own sources, with `-M` for
   the modules, instead of smallcp.
2. Those outputs are assembled and linked into `SMALLC99-stage2.EXE`.
3. Stage 2 must pass the whole regression, and stage 2 compiling the
   sources must give byte-identical output to stage 1 compiling them. This
   is the classic bootstrap proof: the compiler reproduces itself.

From then on a new feature is implemented once, in SMALLC99, and is
available on the PC through the emulator. smallcp becomes the bootstrap of
last resort.

## Testing

- The existing regression (24 programs byte-identical, LANGTEST 133,
  STRUCTTEST 60) after every step.
- A new `PTRTEST.C` (LANGTEST style): `**pp`, `pp[i][j]`, `*tab[i]`, arrays
  of pointers as globals, locals and parameters, `argv`-style walks, pointer
  arithmetic on `char **`, struct pointer arrays, `sizeof`, and depth 2–3.
- Phase B adds `ARRTEST.C`: 2-D and 3-D arrays, row pointers, `sizeof`.

## Space

The resident part has about 150 bytes of slack left inside the linker's
padding, enough for the `elsize()` depth check and the changes to `decl()`.
The expression pages have 270–730 bytes each. If a page runs short,
functions move between the four expression pages, as in the struct work.
