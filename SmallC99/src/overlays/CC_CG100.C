#asm
        AORG 0A000H
#endasm

/*
** CC_CG99.C -- M38e native TMS99000 codegen engine, page 1 of OVL_CGEN
**
** OVL_CGEN is now two pages mapped simultaneously: this >A000 page owns
** ccout(), badcode(), and the hot code[] pointer table. CC_CG99T.C at
** >B000 owns setcodes() and every template literal. Calls/references between
** the two pages are direct because OVLMGR maps both under OVL_CGEN ID 6.
**
** DREL ANCHOR: ccout() -> P_CCOUT. Physical page 11, virtual segment 10.
** The code[] table intentionally stays with ccout(); only initialization and
** template strings moved, so the per-pcode hot path remains on the original page.
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

#define BLPFX  "\tBL @_cc"		/*  shared helper-call prefix   */

badcode(pcode)
int pcode;
{
    if(tmsfail == 0)
        tmsbad = pcode;
    tmsfail = 1;
    errflag = 1;
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
                    pstr(value + NAME);
                }
                else if(*cp == 'n') pdec(value);
                else if(*cp == 'l') pdec(litlab);
                /*  <e> emits R99's external marker.  It cannot be
                    written literally: '#' opens the repeat construct
                    below, so a bare ## in a template is swallowed and
                    the reference assembles as undefined.  */
                else if(*cp == 'e') { pchar('#'); pchar('#'); }
                /*  <b> is the shared call.A99 helper prefix.  Helpers
                    return directly in primary R4; no reload suffix exists.  */
                else if(*cp == 'b') pstr(BLPFX);
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
