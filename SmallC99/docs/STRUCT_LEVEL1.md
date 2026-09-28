# Level-1 structures

This revision adds a compact Level-1 `struct` implementation without changing
LINK99, the historical symbol-record ABI, or the expression descriptor size.
It is built around the machine's actual mapper geometry: a physical page number
is local to a virtual 4K segment, so mapper identity is `(segment,page)`.

## Overlay architecture

`OVL_TYPE` is logical overlay ID 8 and currently owns four co-resident banks:

| Module | Virtual segment | Address | Mapper page | Responsibility |
|---|---:|---:|---:|---|
| `CC_STRUCT_A` | 8 | `>8000` | 15 | tag/declaration parsing and member layout |
| `CC_STRUCT_B` | 9 | `>9000` | 15 | persistent tag/member directory and tag lookup |
| `CC_STRUCT_C` | 10 | `>A000` | 15 | `sizeof(struct ...)`, `.` and `->` lowering |
| `CC_STRUCT_D` | 11 | `>B000` | 15 | member lookup and tag-allocation helpers |

The repeated page number is intentional. `(8,15)`, `(9,15)`, `(10,15)`, and
`(11,15)` are four different mapper banks. They are mapped simultaneously by
one `OVL_TYPE` row, so calls and data references between all four modules are
ordinary direct references. This uses the complete 16K overlay window while
leaving reserve inside every 4K bank instead of crowding one bank.

The expected generated row is:

    WORD 8,15,9,15,10,15,11,15,0,0 ; ID 8

The build audits uniqueness by `(segment,page)`. Reuse of the same page number
in different segments is valid and must not be rejected.

## Compact type representation

The historical symbol record remains unchanged:

    IDENT=0 TYPE=1 CLASS=2 SIZE=3 OFFSET=5 NAME=7

Primitive `TYPE` values remain unchanged. `>40..>5F` (decimal 64..95) encode
32 struct tags directly in the existing one-byte `TYPE` field. This means a
struct object or a pointer-to-struct is self-describing without a resident type
graph, sidecar binding table, or larger expression descriptor.

The tag/member directory is BSS in `CC_STRUCT_B`, therefore it lives in the
mapped aggregate overlay rather than resident/common memory. A selected member
cannot leave the overlay as a pointer because the caller's overlay is restored
on return. `CC_STRUCT_C` therefore copies the historical 23-byte member record
to the one resident scratch record `structtmp`; `is[ST]` can then safely use the
normal fetch/store machinery after `OVL_TYPE` is unmapped.

There is one resident gateway, `R_TYPE`. It maps logical `OVL_TYPE`, calls the
single exported `T_TYPE` dispatcher in `CC_STRUCT_A`, and restores the caller's
logical overlay with the same framed TRSTACK discipline used elsewhere.

## Level-1 language surface

Implemented:

- named structure definitions and forward tag declarations;
- global and automatic structure objects;
- global and automatic pointers to structures;
- K&R function arguments declared as pointers to structures;
- primitive scalar members (`char`, `unsigned char`, `int`, `unsigned int`);
- primitive pointer members and one-dimensional primitive array members;
- `sizeof(struct Tag)`, `sizeof(struct Tag *)`, and `sizeof(struct_object)`;
- direct member selection with `.`;
- indirect member selection with `->`;
- address-of a structure object;
- target-appropriate word alignment of word/pointer members and final size.

Deliberately deferred:

- arrays of structures;
- structure-valued members / nested structures;
- whole-structure assignment;
- structure initializers;
- by-value structure arguments and returns;
- unions and bit-fields;
- generalized pointer arithmetic on structure pointers;
- typedef integration;
- C block-scoped tag namespaces (Level-1 uses one translation-unit tag directory);
- anonymous structure tags.

The deferred items have a natural home in `OVL_TYPE`; they do not require
expanding resident memory or changing LINK99.

## Acceptance fixture

`src/fixtures/STRUCT_L1.C` returns zero on success and numbered failures for
forward tags, object sizes, local/global objects, direct and indirect members,
primitive array members, pointer members, and a K&R structure-pointer argument.
