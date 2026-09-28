#asm
        AORG 09000H
#endasm

/*
** CC_CG99T.C -- M38e TMS99000 template table initializer, page 2 of OVL_CGEN
**
** DREL ANCHOR: setcodes() -> P_SETCODES. Physical page 13, virtual segment 9.
** CC_CG99.C at >8000 and this page are mapped SIMULTANEOUSLY under OVL_CGEN
** ID 6, so setcodes() writes the code[] table on page 1 with an ordinary direct
** external reference, and ccout() later follows pointers to template literals here.
**
** Keeping every literal and the one-shot initializer off the hot ccout() page turns
** the former byte-hunt into a structural 8K codegen budget without changing the
** language, p-code ABI, runtime helpers, trampoline ABI, or OVLMGR implementation.
*/

#define PCODEMAX 75

#define ADD12     1
#define ADDSP     2
#define AND12     3
#define ANEG1     4
#define ARGCNTn   5
#define ASL12     6
#define ASR12     7
#define CALL1     8
#define CALLm     9
#define BYTE_    10
#define BYTEn    11
#define BYTEr0   12
#define COM1     13
#define DBL1     14
#define DBL2     15
#define DIV12    16
#define DIV12u   17
#define ENTER    18
#define EQ10f    19
#define EQ12     20
#define GE10f    21
#define GE12     22
#define GE12u    23
#define POINT1l  24
#define POINT1m  25
#define GETb1m   26
#define GETb1mu  27
#define GETb1p   28
#define GETb1pu  29
#define GETw1m   30
#define GETw1n   31
#define GETw1p   32
#define GETw2n   33
#define GT10f    34
#define GT12     35
#define GT12u    36
#define WORD_    37
#define WORDn    38
#define WORDr0   39
#define JMPm     40
#define LABm     41
#define LE10f    42
#define LE12     43
#define LE12u    44
#define LNEG1    45
#define LT10f    46
#define LT12     47
#define LT12u    48
#define MOD12    49
#define MOD12u   50
#define MOVE21   51
#define MUL12    52
#define MUL12u   53
#define NE10f    54
#define NE12     55
#define NEARm    56
#define OR12     57
#define POINT1s  58
#define POP2     59
#define PUSH1    60
#define PUTbm1   61
#define PUTbp1   62
#define PUTwm1   63
#define PUTwp1   64
#define rDEC1    65
#define REFm     66
#define RETURN   67
#define rINC1    68
#define SUB12    69
#define SWAP12   70
#define SWAP1s   71
#define SWITCH   72
#define XOR12    73
#define LSR12    74

extern char *code[];

setcodes()
{
    int i;

    i = 0;
    while(i < PCODEMAX) code[i++] = 0;

    /* The templates are split across setcodes1..7 so each function's
    ** string literals fit SMALLC99's 255-byte literal queue (one function
    ** held 1,430 bytes; SMALLC99 compiling its own sources needs this). */
    setcodes1();
    setcodes2();
    setcodes3();
    setcodes4();
    setcodes5();
    setcodes6();
    setcodes7();
}

setcodes1()
{

    /* Arithmetic, logic, shifts, calls, and stack operations. */
    code[ADD12]   = ".\tA R3,R4\n";
    code[ADDSP]   = ".?\tAI SP,<n>\n??";
    code[AND12]   = ".\tINV R3\n\tSZC R3,R4\n";
    code[ANEG1]   = ".\tNEG R4\n";
    code[ARGCNTn] = ".?\tLI R5,<n>?\tCLR R5?\n";
    code[ASL12]   = ".<b>asl<e>\n";
    code[ASR12]   = ".<b>asr<e>\n";
    /* Unsigned >> uses the existing zero-count-safe logical-shift helper. */
    code[LSR12]   = ".<b>usr<e>\n";
    code[CALL1]   = ".\tCALL *R4\n";
    code[CALLm]   = ".\tCALL @<m>\n";
    code[COM1]    = ".\tINV R4\n";
    code[DBL1]    = ".\tA R4,R4\n";
    code[DBL2]    = ".\tA R3,R3\n";
    code[MOVE21]  = ".\tMOV R4,R3\n";
    code[OR12]    = ".\tSOC R3,R4\n";
    code[POP2]    = ".\tMOV *SP+,R3\n";
    code[PUSH1]   = ".\tDECT SP\n\tMOV R4,*SP\n";
}

setcodes2()
{
    code[rDEC1]   = ".#\tDEC R4\n#";
    code[rINC1]   = ".#\tINC R4\n#";
    code[SUB12]   = ".\tS R4,R3\n\tMOV R3,R4\n";
    code[SWAP12]  = ".\tMOV R3,R0\n\tMOV R4,R3\n\tMOV R0,R4\n";
    code[SWAP1s]  = ".\tMOV *SP,R0\n\tMOV R4,*SP\n\tMOV R0,R4\n";
    code[XOR12]   = ".\tXOR R3,R4\n";

    /*  call.A99 uses the native Small-C register convention: R3 is
        secondary/left, R4 is primary/right/result. Signed DIV/MOD continue
        through _ccdiv. The active CC_CD99 generator does NOT pre-swap them.

        TMS9900 DIV is unsigned. Use the same proven register arrangement as
        _ccudiv: copy the 16-bit dividend from R3 into the low half R2, clear
        high half R1, and divide R1:R2 by divisor R4. DIV leaves quotient in
        R1 and remainder in R2. */
    code[MUL12]   = ".<b>mult<e>\n";
    code[MUL12u]  = code[MUL12];
    code[DIV12]   = ".<b>div<e>\n";
    code[DIV12u]  = ".\tCLR R1\n\tMOV R3,R2\n\tDIV R4,R1\n\tMOV R1,R4\n";
}

setcodes3()
{
    code[MOD12]   = ".<b>div<e>\n\tMOV R3,R0\n\tMOV R4,R3\n\tMOV R0,R4\n";
    code[MOD12u]  = ".\tCLR R1\n\tMOV R3,R2\n\tDIV R4,R1\n\tMOV R2,R4\n";
    code[EQ12]    = ".<b>eq<e>\n";
    code[NE12]    = ".<b>ne<e>\n";
    code[LT12]    = ".<b>lt<e>\n";
    code[LE12]    = ".<b>le<e>\n";
    code[GT12]    = ".<b>gt<e>\n";
    code[GE12]    = ".<b>ge<e>\n";
    code[LT12u]   = ".<b>ult<e>\n";
    code[LE12u]   = ".<b>ule<e>\n";
    code[GT12u]   = ".<b>ugt<e>\n";
    code[GE12u]   = ".<b>uge<e>\n";
    code[LNEG1]   = ".<b>lneg<e>\n";
    /* _ccswitc is special: unlike the arithmetic helpers, it consumes the
       XOP6 CALL return word from the software stack and uses that address as
       the inline switch-table pointer.  It therefore MUST use CALL, not BL. */
    code[SWITCH]  = ".\tCALL @_ccswitc<e>\n";

    /*  Branch when the condition is FALSE.  These test PRIMARY R4
        against zero - zerojump() has already
        purged the "oper 0" comparison.  B is two words, so the skip is
        $+6; GE and LE need two jumps because the 99105 has no JGE/JLE.
        Displacements verified through A99.  */
}

setcodes4()
{
    code[EQ10f]    = ".\tMOV R4,R4\n\tJEQ $+6\n\tB @_<n>\n";
    code[NE10f]    = ".\tMOV R4,R4\n\tJNE $+6\n\tB @_<n>\n";
    code[LT10f]    = ".\tMOV R4,R4\n\tJLT $+6\n\tB @_<n>\n";
    code[GT10f]    = ".\tMOV R4,R4\n\tJGT $+6\n\tB @_<n>\n";
    code[GE10f]    = ".\tMOV R4,R4\n\tJGT $+8\n\tJEQ $+6\n\tB @_<n>\n";
    code[LE10f]    = ".\tMOV R4,R4\n\tJLT $+8\n\tJEQ $+6\n\tB @_<n>\n";

}

setcodes5()
{
    /* Function entry and return. */
    code[ENTER]   = ".\tSTWP WP\n\tDECT SP\n\tMOV FP,*SP\n\tMOV SP,FP\n";
    code[RETURN]  = ".\tMOV FP,SP\n\tMOV *SP+,FP\n\tRET\n";

    /* Addresses and word/byte memory access. */
    code[POINT1l] = ".\tLI R4,_<l>+<n>\n";
    code[POINT1m] = ".\tLI R4,<m>\n";
    code[POINT1s] = ".\tMOV FP,R4\n?\tAI R4,<n>\n??";

    code[GETw1m]  = ".\tMOV @<m>,R4\n";
    code[GETw1n]  = ".?\tLI R4,<n>?\tCLR R4?\n";
    code[GETw1p]  = ".?\tMOV @<n>(R3),R4?\tMOV *R3,R4?\n";
    code[GETw2n]  = ".?\tLI R3,<n>?\tCLR R3?\n";
}

setcodes6()
{
    code[PUTwm1]  = ".\tMOV R4,@<m>\n";
    code[PUTwp1]  = ".\tMOV R4,*R3\n";

    code[GETb1m]  = ".\tMOVB @<m>,R4\n\tSRA R4,8\n";
    code[GETb1mu] = ".\tMOVB @<m>,R4\n\tSRL R4,8\n";
    code[GETb1p]  = ".?\tMOVB @<n>(R3),R4?\tMOVB *R3,R4?\n\tSRA R4,8\n";
    code[GETb1pu] = ".?\tMOVB @<n>(R3),R4?\tMOVB *R3,R4?\n\tSRL R4,8\n";
    code[PUTbm1]  = ".\tMOVB @2*R4+1(WP),@<m>\n";
    code[PUTbp1]  = ".\tMOVB @2*R4+1(WP),*R3\n";

    /* Native R99 data and label emitters. */
    code[BYTE_]   = ".\tBYTE ";
    code[BYTEn]   = ".\tBYTE <n>\n";
}

setcodes7()
{
    code[BYTEr0]  = ".\tBSS <n>\n";
    code[WORD_]   = ".\tWORD ";
    code[WORDn]   = ".\tWORD <n>\n";
    code[WORDr0]  = ".\tBSS <n>\n";
    code[NEARm]   = ".\tWORD _<n>\n";
    code[REFm]    = "._<n>";
    code[JMPm]    = ".\tB @_<n>\n";
    code[LABm]    = "._<n>:\n";
}

