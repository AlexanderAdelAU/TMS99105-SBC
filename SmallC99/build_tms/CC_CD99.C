/*
** CC_CD99.C -- M38d resident TMS99000 output and staging services
**
** This is linked only into SMALLC99.EXE. The blessed 8086 executable keeps
** CC_CODEGEN.C unchanged. The staging and p-code accounting are the Hendrix
** machinery; only the module formatter and paged template implementation are
** target-specific.
*/

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
#define GETw2m   89

#define NAME      7
#define BPW       2
#define STAGESZ 400

#define IDENT     0
#define CLASS     2
#define SYMMAX   16
#define SYMAVG   12
#define NUMLOCS  25
#define NUMGLBS 200
#define FUNCTION  4
#define AUTOEXT   4

#define STARTGLB (symtab + NUMLOCS * SYMAVG)
#define ENDGLB   (STARTGLB + NUMGLBS * SYMMAX)

extern int csp;
extern int oldseg;
extern char symtab[];
extern char ssname[];
extern char *cptr;
extern R_CCOUT();
extern R_SETCODES();
extern findglb();

int stage[STAGESZ];
int *snext;
int tmsfail;
int tmsbad;

setcodes()
{
    tmsfail = 0;
    tmsbad = 0;
    R_SETCODES();
}

gen(pcode, value)
int pcode;
int value;
{
    int newcsp;

    if(pcode == GETb1pu || pcode == GETb1p || pcode == GETw1p)
        gen(MOVE21, 0);
    else if(pcode == SUB12 || pcode == MOD12 || pcode == MOD12u ||
            pcode == DIV12 || pcode == DIV12u)
        gen(SWAP12, 0);
    else if(pcode == PUSH1)
        csp -= BPW;
    else if(pcode == POP2)
        csp += BPW;
    else if(pcode == ADDSP || pcode == RETURN) {
        newcsp = value;
        value = value - csp;
        csp = newcsp;
    }

    if(snext == 0) {
        R_CCOUT(pcode, value);
        return;
    }

    if(snext >= stage + STAGESZ) {
        error("staging buffer overflow");
        return;
    }

    snext[0] = pcode;
    snext[1] = value;
    snext = snext + 2;
}

setstage(before, start)
int *before;
int *start;
{
    if((*before = snext) == 0)
        snext = stage;
    *start = snext;
}

clearstage(before, start)
int *before;
int *start;
{
    if(before) {
        snext = before;
        return;
    }
    if(start)
        dumpstage();
    snext = 0;
}

dumpstage()
{
    int *end;

    end = snext;
    snext = stage;

    while(snext < end) {
        R_CCOUT(snext[0], snext[1]);
        snext = snext + 2;
    }
}

outline(ptr)
char *ptr;
{
    pstr(ptr);
    pnl();
}

outname(ptr)
char *ptr;
{
    pchar('_');
    while(*ptr >= ' ')
        pchar(*ptr++);
}

/* Native R99 source has no 8086 CODE/DATA segment switching. */
toseg(seg)
int seg;
{
    oldseg = seg;
}

public(ident)
int ident;
{
    if(ident == FUNCTION) {   /* TMS: code must start even */
        pstr("\tEVEN");
        pnl();
    }
    pstr("\tENT ");
    outname(ssname);
    pnl();
    outname(ssname);
    if(ident == FUNCTION) {
        pchar(':');
        pnl();
    }
}

external(name, size, ident)
char *name;
int size;
int ident;
{
    pstr("\tEXT ");
    outname(name);
    pnl();
}

header()
{
    outline("; SMALLC99 2.2 M38d native TMS99000 output");
    outline("R0\tEQU 0");
    outline("R1\tEQU 1");
    outline("R2\tEQU 2");
    outline("R3\tEQU 3");
    outline("R4\tEQU 4");
    outline("R5\tEQU 5");
    outline("FP\tEQU 9");
    outline("SP\tEQU 10");
    outline("R11\tEQU 11");
    outline("WP\tEQU 13");
}

trailer()
{
    if(tmsfail) {
        outline("; COMPILATION FAILED - unsupported TMS p-code");
        return;
    }

    cptr = STARTGLB;
    while(cptr < ENDGLB) {
        if(cptr[IDENT] == FUNCTION && cptr[CLASS] == AUTOEXT)
            external(cptr + NAME, 0, FUNCTION);
        cptr = cptr + SYMMAX;
    }

    outline("\tEVEN");
    outline("\tEND");
}
