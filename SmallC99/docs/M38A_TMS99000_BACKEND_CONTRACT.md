# M38a - SmallC99 TMS99000 backend contract

## Status and scope

M38a is a design-only milestone based on the blessed M37a r2 project. It does
not alter, select, or replace the working 8086 backend. It freezes the machine
contract that the first TMS99000 backend implementation must follow.

The retargeting boundary remains the existing 73-operation Hendrix p-code
interface. The front end, parser, preprocessor, declarations, expression
semantics, optimiser and overlay manager remain common. The TMS backend will
supply a new template table, native module formatter and runtime helpers.

## Canonical register names

Generated source will include `src/tms99000/TMSREGS.INC` or emit an equivalent
preamble. The canonical software-stack spelling is:

```asm
SP      EQU     10
```

Target templates must use `SP`, not `R10`.

| Register | Backend role | Preservation rule |
|---|---|---|
| R0-R2 | Backend and runtime scratch | Caller-clobbered |
| R3 | Primary accumulator and C return value | Caller-clobbered/result |
| R4 | Secondary operand | Caller-clobbered |
| R5 | Argument count and helper input | Caller-clobbered |
| R6-R8 | Reserved for later backend optimisation | Preserved by helpers |
| R9 | Frame pointer | Preserved by generated functions |
| SP | Software stack pointer (`EQU 10`) | Restored on return |
| R11 | `BL` link register | Saved by any generated function/helper that calls |
| R12 | Platform/CRU reserved | Preserved |
| R13-R15 | Context/platform reserved | Preserved |

The conservative helper clobber set is R0-R5 and R11. Helpers preserve R6-SP
and R12-R15, and return word results in R3.

## Stack and call contract

The software stack grows downward in two-byte units. Arguments retain the
existing Hendrix order: the caller pushes them from left to right, sets R5 to
the argument count, performs `BL`, and removes the arguments after return.

```asm
DECT SP
MOV  R3,*SP             ; PUSH1

BL   @function
AI   SP,argument_bytes  ; caller cleanup
```

A generated function uses this prologue:

```asm
DECT SP
MOV  R11,*SP            ; saved return address
DECT SP
MOV  R9,*SP             ; saved frame pointer
MOV  SP,R9
```

The resulting frame layout deliberately matches the current 8086 backend:

```text
0(R9)   saved R9
2(R9)   saved R11
4(R9)   last argument
6(R9)   preceding argument
...
```

The epilogue is:

```asm
MOV  R9,SP
MOV  *SP+,R9
MOV  *SP+,R11
B    *R11
```

This keeps the existing argument and local offsets unchanged in the front end.

## Word, byte and Boolean representation

C `int` and pointers are 16-bit words. The machine is big-endian. A C byte held
in R3 is right-justified in the low eight bits:

```asm
; unsigned byte fetch through R4
CLR  R3
MOVB *R4,R3
SWPB R3

; signed byte fetch through R4
MOVB *R4,R3
SRA  R3,8
```

A byte store must store R3's low byte while preserving R3, because assignment
expressions retain the assigned value:

```asm
MOV  R3,R0
SWPB R0
MOVB R0,*R4
```

Boolean false is zero and true is one. Comparison and logical helpers return
that representation in R3.

## Address versus value

The backend must preserve the distinction between forming a local address and
loading a local value:

```asm
; address of local at offset -4
MOV  R9,R3
AI   R3,-4

; value of local at offset -4
MOV  @-4(R9),R3
```

There is no assumed `LA` instruction.

## Arithmetic and logic conventions

Binary p-codes receive the primary operand in R3 and secondary operand in R4.
R4 may be destroyed unless an individual p-code states otherwise.

```asm
A    R4,R3              ; R3 = R3 + R4
S    R4,R3              ; R3 = R3 - R4
SOC  R4,R3              ; R3 = R3 OR R4
XOR  R4,R3              ; R3 = R3 XOR R4
INV  R4
SZC  R4,R3              ; R3 = R3 AND original R4
```

Signed/unsigned multiply, divide, remainder, comparisons, logical negation and
variable shifts begin as runtime helpers. This is intentional: it makes count
ranges, overflow and signedness explicit before any inline optimisation.

## Native assembler output

The future TMS module formatter will replace the M37a 8086 module formatter. It
will emit native R99 assembler constructs such as `IDT`, `ENT`, `EXT`, `DATA`,
`BYTE`, `EVEN` and `END`, following the conventions already accepted by this
project's assembler/linker.

The exact zero-fill directive for `BYTEr0` and `WORDr0` is deliberately not
frozen until the linker/load behaviour of `BSS` is demonstrated. The semantic
requirement is explicit zero initialisation.

## Complete p-code authority

`src/tms99000/PCODE99.MAP` records all 73 p-codes, their semantic inputs and
outputs, initial direct lowering or runtime helper, clobbers and unresolved
assembler-format questions. No p-code may be implemented by translating an
8086 mnemonic without checking that map.

## Planned implementation sequence

1. **M38b:** separate TMS template/output modules; compile `main(){return 9;}`.
2. **M38c:** stack frames, locals, globals, direct calls and caller cleanup.
3. **M38d:** arithmetic, logic, branches and comparison/runtime helpers.
4. **M38e:** bytes, pointers, strings, literal pools and initialised data.
5. **M38f:** switch tables, indirect calls, include files and inline assembly.
6. **M38g:** assemble, link and execute the existing M36 fixture suite.

## M38a acceptance

M38a is accepted when review confirms:

- `SP EQU 10` is the canonical generated spelling;
- the R3/R4/R5/R9/SP/R11 roles are agreed;
- the frame layout matches the blessed 8086 offsets;
- byte values are right-justified in registers;
- address formation and value loads remain distinct;
- every p-code from 1 through 73 appears exactly once in `PCODE99.MAP`;
- the blessed M37a r2 build is byte-for-byte unaffected.
