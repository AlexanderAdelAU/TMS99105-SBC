# Type and aggregate architecture

## Why this exists

Level-1 structures are implemented as the first client of a general type layer, not as struct-specific code distributed through the declaration and expression overlays.  The previous experimental implementation overflowed `CC_EXPR_B`; moving helpers into spare bytes in another expression page would only postpone the same problem.  This design gives future aggregate/type work an explicit home and an explicit budget.

## Non-negotiable compatibility rule

The historical Small-C symbol record is unchanged.  Existing `IDENT`, `TYPE`, `CLASS`, `SIZE`, `OFFSET`, and `NAME` fields keep their layout and legacy meanings.  Symbols that require information the old record cannot express are associated with a resident general type reference through a sidecar binding table.  Primitive-only symbols need no sidecar entry.

This keeps the ABI used by the scanner, declaration overlays, code generator, and old symbol-table walkers intact.

## Type descriptors

Resident descriptor arrays represent these kinds:

- scalar
- pointer
- array
- struct
- union (representation reserved; syntax/semantics are not enabled at Level 1)

Built-in `char`, `unsigned char`, `int`, and `unsigned int` have stable initial type references.  Pointer and array descriptors refer to another type descriptor, so composition does not require another symbol-record redesign.  Aggregate descriptors refer to a tag record.

An aggregate tag record owns its name, kind, completeness, size, and member range.  A member record retains the historical lvalue-facing fields plus a full type reference.  That split lets the existing fetch/store machinery continue to operate while richer semantics travel through the new `TR` expression field.

## Ownership

`OVL_TYPE` (ID 8) is a two-page logical overlay. `CC_TYPE_A.C` is mapped at virtual segment 8 / physical page 15 and `CC_TYPE_B.C` at virtual segment 9 / physical page 1. Both pages are mapped simultaneously, exactly like the multi-page expression/codegen overlays, so calls between the two type pages are ordinary direct calls. It owns:

- type-specifier parsing
- tag creation/lookup
- aggregate definition and layout
- member lookup
- conversion of a resolved member into the normal expression lvalue descriptor

Resident code owns the persistent tables and a deliberately small **type kernel**: descriptor allocation/deduplication, pointer/array composition, tiny type queries, symbol/type binding, and framed bridges into `OVL_TYPE`. Descriptor construction stays resident because its storage is resident and declarators use it frequently; it does not page-swap merely to create a pointer or array type. Tag/member name lookup and layout remain exclusively in `OVL_TYPE`; they are not duplicated in resident code.

Declaration, statement, function-argument, and expression overlays call the type layer rather than implementing aggregate lookup themselves.

## Declaration seam

Declarations now begin with a general type reference.  Storage/declarator code then adds pointer or array shape and writes the legacy symbol record.  Rich final types are bound to a symbol only when they contain an aggregate.

This is the intended seam for later `union`, nested aggregates, richer pointer composition, and eventually `typedef`: extend type parsing/descriptors rather than add another parallel declaration grammar.

## Expression seam

The expression lvalue descriptor has one new field, `TR`, carrying the general type reference.  Primary expressions load it from the symbol sidecar.  Ordinary scalar code continues to use the old fields.

For `.` and `->`, the expression page only recognizes the postfix operator, prepares the aggregate base address, and calls the type layer.  `OVL_TYPE` resolves the member, adds its byte offset, and returns a normal lvalue descriptor.  Existing fetch/store code then handles scalar, pointer, and array members.

Arithmetic/logical operations clear or propagate `TR` deliberately so a scalar result cannot retain a stale aggregate type.

## Capacity and growth policy

Current source-level table caps are deliberately explicit (`TYPEMAX`, `TYPEBINDMAX`, `AGGMAX`, `AGGMEMMAX`).  They are compile-time compiler-workspace limits, not object ABI limits.

The build requires **each** OVL_TYPE page to retain at least `>0300` bytes below LINK99's `>0FC0` chain-safe page limit. The v3 single-page experiment assembled to `>0FAC` (4012 bytes), leaving only 20 bytes; that result triggered the reserve guard and is why the subsystem is now two pages rather than packed into unrelated compiler overlays.

Physical page allocation is a first-class constraint. Pages 15 and 1 are dedicated to `OVL_TYPE`. Page A owns type-specifier parsing and aggregate definition/layout; page B owns tag/member lookup and member-expression normalization. Future growth should preserve those responsibilities or add mapped type-system storage explicitly.

## Intended extension path

The data model is prepared for, but Level 1 does not yet implement, union syntax, nested aggregate members, arrays of structs, aggregate initializers, whole-aggregate assignment, aggregate function arguments/returns, bit-fields, or typedef names.  Those features should extend `OVL_TYPE` and generic type/declarator rules while keeping expression hooks small.
