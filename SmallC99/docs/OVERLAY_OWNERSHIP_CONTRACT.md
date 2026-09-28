# SMALLC99 overlay ownership contract

## The invariant

The framed overlay stubs save and restore one logical window owner: `CUR_WINA_ID`.
`OVLMGR` assigns that owner when an overlay maps virtual segment 8. Therefore a
normal/nestable overlay must map segment 8, and segment 8 must be its first table
entry.

In short:

```text
segment-8 owner == CUR_WINA_ID == logical active overlay
```

A nested service overlay must not execute only from segments A/B while another
overlay continues to own segment 8. That creates mixed ownership which cannot be
represented by the single saved `CUR_WINA_ID` used by the framed stubs.

## CGEN regression

The bad CGEN topology was:

```text
OVL_EXPR: 8=4, 9=5, A=7, B=8
OVL_CGEN: A=11, B=13
```

CGEN could therefore execute while `CUR_WINA_ID` still identified EXPR.

The corrected topology is:

```text
OVL_EXPR: 8=4, 9=5, A=7, B=8
OVL_CGEN: 8=11, 9=13
```

Mapping CGEN now makes ID 6 the logical window owner. Restoring the saved caller
ID then restores the complete expression mapping.

## Startup invariant

`PORT_INIT` must establish a known logical owner before any overlay dispatch:

```asm
CALL @OVLMGR_INIT
CLR  @CUR_WINA_ID
CALL @STUBINIT
```

This prevents a previous invocation's owner ID from being consumed as current
state.

## Build-time enforcement

`make_smallc99.ps1` fails the build if any of these conditions is violated:

1. Every generated overlay row except explicit one-shot exceptions starts with
   segment 8 and maps segment 8 exactly once.
2. CGEN ID 6 is exactly `8,11,9,13,0,0,0,0,0,0`.
3. `P_CCOUT` resolves inside `>8000..>8FFF`.
4. `P_SETCODES` resolves inside `>9000..>9FFF`.
5. `PORT_INIT` does not clear `CUR_WINA_ID` immediately after `OVLMGR_INIT` and
   before `STUBINIT`.
6. CLI ID 7 remains the documented top-level one-shot exception.

A future overlay that genuinely cannot own segment 8 must not simply be placed on
the exception list. Its call/restore semantics must first be shown not to use the
framed `CUR_WINA_ID` ownership model. Exceptions are architecture decisions, not
space-allocation conveniences.

## Maintenance question

Whenever an overlay is added or moved, ask:

> Who owns segment 8 while this code is executing, and does `CUR_WINA_ID` agree?

The build now asks the same question automatically for the generated overlay
table.

The guard does not alter the OVLMGR mapping algorithm or the framed stub semantics.

## Deterministic regression proof

The ownership invariant was exercised directly with temporary runtime assertions.
With the corrected CGEN 8/9 layout, STRESS completed with 713 successful CGEN
ownership checks and 712 successful non-zero caller restores (`C60002C9` /
`C60102C8`). Repeating the run produced the same counts. Restoring only the old
A/B CGEN topology caused the first CGEN transition to fail immediately with
`C6E10000`, proving that the old layout did not establish CGEN as the segment-8
owner. The temporary runtime proof instrumentation is not part of production.
