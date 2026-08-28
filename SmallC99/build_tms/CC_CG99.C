#asm
        AORG 0A000H
#endasm

/*
** CC_CG99.C -- M38d native TMS99000 byte/word template overlay
**
** M38d retains the M38c r1 word operations and adds ED2-derived
** byte movement: WP caches the current workspace, byte stores read R3's
** low byte directly from workspace memory, and byte loads normalize to a
** 16-bit value in R3. Unsupported operations still fail visibly.
*/

#define PCODEMAX 74
#define NAME     7
#define YES      1
#define NO       0

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

extern int litlab;
extern int errflag;
extern int tmsfail;
extern int tmsbad;
extern pstr();
extern pchar();
extern pdec();
extern pnl();
extern error();

char *code[PCODEMAX];

setcodes()
{
    int i;

    i = 0;
    while(i < PCODEMAX) code[i++] = 0;

    /* Arithmetic, logic, shifts, calls, and stack operations. */
    code[ADD12]   = ".\tA R4,R3\n";
    code[ADDSP]   = ".?\tAI SP,<n>\n??";
    code[AND12]   = ".\tINV R4\n\tSZC R4,R3\n";
    code[ANEG1]   = ".\tNEG R3\n";
    code[ARGCNTn] = ".?\tLI R5,<n>?\tCLR R5?\n";
    code[ASL12]   = ".\tMOV R3,R0\n\tMOV R4,R3\n\tSLA R3,0\n";
    code[ASR12]   = ".\tMOV R3,R0\n\tMOV R4,R3\n\tSRA R3,0\n";
    code[CALL1]   = ".\tBL *R3\n";
    code[CALLm]   = ".\tBL @<m>\n";
    code[COM1]    = ".\tINV R3\n";
    code[DBL1]    = ".\tA R3,R3\n";
    code[DBL2]    = ".\tA R4,R4\n";
    code[MOVE21]  = ".\tMOV R3,R4\n";
    code[OR12]    = ".\tSOC R4,R3\n";
    code[POP2]    = ".\tMOV *SP+,R4\n";
    code[PUSH1]   = ".\tDECT SP\n\tMOV R3,*SP\n";
    code[rDEC1]   = ".#\tDEC R3\n#";
    code[rINC1]   = ".#\tINC R3\n#";
    code[SUB12]   = ".\tS R4,R3\n";
    code[SWAP12]  = ".\tMOV R3,R0\n\tMOV R4,R3\n\tMOV R0,R4\n";
    code[SWAP1s]  = ".\tMOV *SP,R0\n\tMOV R3,*SP\n\tMOV R0,R3\n";
    code[XOR12]   = ".\tXOR R4,R3\n";

    /* Function entry and return. */
    code[ENTER]   = ".\tSTWP WP\n\tDECT SP\n\tMOV R11,*SP\n\tDECT SP\n\tMOV FP,*SP\n\tMOV SP,FP\n";
    code[RETURN]  = ".\tMOV FP,SP\n\tMOV *SP+,FP\n\tMOV *SP+,R11\n\tB *R11\n";

    /* Addresses and word/byte memory access. */
    code[POINT1l] = ".\tLI R3,_<l>+<n>\n";
    code[POINT1m] = ".\tLI R3,<m>\n";
    code[POINT1s] = ".\tMOV FP,R3\n?\tAI R3,<n>\n??";

    code[GETw1m]  = ".\tMOV @<m>,R3\n";
    code[GETw1n]  = ".?\tLI R3,<n>?\tCLR R3?\n";
    code[GETw1p]  = ".?\tMOV @<n>(R4),R3?\tMOV *R4,R3?\n";
    code[GETw2n]  = ".?\tLI R4,<n>?\tCLR R4?\n";
    code[PUTwm1]  = ".\tMOV R3,@<m>\n";
    code[PUTwp1]  = ".\tMOV R3,*R4\n";

    code[GETb1m]  = ".\tMOVB @<m>,R3\n\tSRA R3,8\n";
    code[GETb1mu] = ".\tMOVB @<m>,R3\n\tSRL R3,8\n";
    code[GETb1p]  = ".?\tMOVB @<n>(R4),R3?\tMOVB *R4,R3?\n\tSRA R3,8\n";
    code[GETb1pu] = ".?\tMOVB @<n>(R4),R3?\tMOVB *R4,R3?\n\tSRL R3,8\n";
    code[PUTbm1]  = ".\tMOVB @2*R3+1(WP),@<m>\n";
    code[PUTbp1]  = ".\tMOVB @2*R3+1(WP),*R4\n";

    /* Native R99 data and label emitters. */
    code[BYTE_]   = ".\tBYTE ";
    code[BYTEn]   = ".\tBYTE <n>\n";
    code[BYTEr0]  = ".\tBSS <n>\n";
    code[WORD_]   = ".\tWORD ";
    code[WORDn]   = ".\tWORD <n>\n";
    code[WORDr0]  = ".\tBSS <n>\n";
    code[NEARm]   = ".\tWORD _<n>\n";
    code[REFm]    = "._<n>";
    code[JMPm]    = ".\tB @_<n>\n";
    code[LABm]    = "._<n>:\n";
}

badcode(pcode)
int pcode;
{
    if(tmsfail == 0)
        tmsbad = pcode;
    tmsfail = 1;
    errflag = 1;
    error("unsupported TMS p-code");
    pstr("; ERROR unsupported TMS p-code ");
    pdec(pcode);
    pnl();
}

ccout(pcode, value)
int pcode;
int value;
{
    int part;
    int skip;
    int count;
    char *cp;
    char *back;
    char *tp;

    if(pcode < 1 || pcode >= PCODEMAX) {
        badcode(pcode);
        return;
    }

    tp = code[pcode];
    if(tp == 0) {
        badcode(pcode);
        return;
    }

    part = 0;
    back = 0;
    skip = NO;
    cp = tp + 1;

    while(*cp) {
        if(*cp == '<') {
            ++cp;
            if(skip == NO) {
                if(*cp == 'm') {
                    pchar('_');
                    pstr(value + NAME);
                }
                else if(*cp == 'n') pdec(value);
                else if(*cp == 'l') pdec(litlab);
            }
            cp += 2;
        }
        else if(*cp == '?') {
            ++part;
            if(part == 1) {
                if(value == 0) skip = YES;
            }
            else if(part == 2) skip = !skip;
            else if(part == 3) {
                part = 0;
                skip = NO;
            }
            ++cp;
        }
        else if(*cp == '#') {
            ++cp;
            if(back == 0) {
                count = value;
                if(count < 1) {
                    while(*cp && *cp++ != '#') ;
                }
                else back = cp;
            }
            else {
                --count;
                if(count > 0) cp = back;
                else back = 0;
            }
        }
        else if(skip == NO) pchar(*cp++);
        else ++cp;
    }
}
