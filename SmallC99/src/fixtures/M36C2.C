/* M36c2 inline-assembly passthrough fixture. */

#include "M36ASM.H"

#asm
ASM2TOP:
    MOV AX,123
; M36C2 blank line follows

    MOV BX,456
#endasm

int result;

main()
{
#asm
ASM2B1:
    XCHG AX,BX
#endasm

    result=asmword+ASMBONUS;

#asm
ASM2B2:
    NOP
#endasm

    return result;
}
