#asm
	AORG 8000H
#endasm

/*
** CC_EXPR_A -- CCC3 expression engine, page 1 of a 4-page 16K window
**
** The engine measured 13656 bytes. A 4KB page cannot hold it and an
** 8KB pair could not either, so window A is now FOUR pages --
** >8000-BFFF, segments 8..11 -- mapped SIMULTANEOUSLY by OVLMGR.
** All four are addressable at once, so every call between them is an
** ordinary direct call. The split points are therefore chosen purely
** by size and by DREL anchoring, not by any call structure.
**
** ROOM CAME FROM THE RESIDENT SIDE, not from trimming the engine.
** Dropping the CLIB printf closure (4714 bytes, replaced by ~200
** bytes of SPYOUT.A99 on the monitor WRITE XOP) and the file I/O
** modules nothing yet calls (5142 bytes) freed 9856 bytes. That let
** codegen become resident, which released its whole segment to the
** window. The engine ships verbatim: nothing trimmed, no helpers
** relocated, sizeof and ?: intact.
**
** DREL ANCHOR: constexpr(). DREL tags a module by recognising an exported
** entry name; a module it cannot tag gets no OVL_TABLE row, its page
** is never mapped, and the first call into it lands in unmapped
** memory. Each page carries exactly one anchor and none may be moved
** without providing a replacement.
**
** DO NOT write "extern name();" for a function on another page.
** smallcp emits an ENT for that, this module exports its own stub,
** and the call binds to the stub instead of the real function --
** silent at link time, a wild call at run time. Undeclared calls
** emit a clean EXT, which is what is wanted.
*/
/* ---- lvalue array "is[7]" -- CCC3 field offsets ---- */
#define ST 0    /* is[ST] symbol table address, else 0                */
#define TI 1    /* is[TI] type of indirect object to fetch, else 0    */
#define TA 2    /* is[TA] type of address, else 0                     */
#define TC 3    /* is[TC] type of constant (INT or UINT), else 0      */
#define CV 4    /* is[CV] value of constant (+ auxiliary uses)        */
#define OP 5    /* is[OP] code of highest/last binary operator        */
#define SA 6    /* is[SA] stage address of "op 0" code, else 0        */

/* ---- machine ---- */
#define BPW      2
#define LBPW     1

/* ---- symbol table format ---- */
#define IDENT    0
#define TYPE     1
#define CLASS    2
#define SIZE     3
#define OFFSET   5
#define NAME     7
#define NAMESIZE 9

/* ---- values for IDENT ---- */
#define LABEL    0
#define VARIABLE 1
#define ARRAY    2
#define POINTER  3
#define FUNCTION 4

/* ---- values for TYPE ---- */
#define CHR      4
#define INT      8
#define UCHR     5
#define UINT     9
#define UNSIGNED 1

/* ---- values for CLASS ---- */
#define AUTOMATIC 1
#define STATIC    2
#define EXTERNAL  3
#define AUTOEXT   4

/* ---- p-codes, from CC.H ---- */
#define ADD12     1
#define ADDSP     2
#define AND12     3
#define ANEG1     4
#define ARGCNTn   5
#define ASL12     6
#define ASR12     7
#define CALL1     8
#define CALLm     9
#define COM1     13
#define DBL1     14
#define DBL2     15
#define DIV12    16
#define DIV12u   17
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
#define XOR12    73

extern char *glbptr;
extern char *lptr;
extern char litq[];
extern char ssname[NAMESIZE];
extern char quote[2];

extern int ch;
extern int nch;
extern int csp;
extern int litlab;
extern int litptr;
extern int op[16];
extern int op2[16];
extern int opindex;
extern int opsize;
extern int *snext;
/***************** lead-in functions *******************/

constexpr(val) int *val; {
  int const;
  int *before, *start;
  setstage(&before, &start);
  expression(&const, val);
  clearstage(before, 0);     /* scratch generated code */
  if(const == 0) error("must be constant expression");
  return const;
  }

expression(con, val) int *con, *val;  {
  int is[7];
  if(level1(is)) fetch(is);
  *con = is[TC];
  *val = is[CV];
  }

level13(is)  int is[];  {
  int k;
  char *ptr;
  if(match("++")) {                 /* ++lval */
    if(level13(is) == 0) {
      needlval();
      return 0;
      }
    step(rINC1, is, 0);
    return 0;
    }
  else if(match("--")) {            /* --lval */
    if(level13(is) == 0) {
      needlval();
      return 0;
      }
    step(rDEC1, is, 0);
    return 0;
    }
  else if(match("~")) {             /* ~ */
    if(level13(is)) fetch(is);
    gen(COM1, 0);
    is[CV] = ~ is[CV];
    return (is[SA] = 0);
    }
  else if(match("!")) {             /* ! */
    if(level13(is)) fetch(is);
    gen(LNEG1, 0);
    is[CV] = ! is[CV];
    return (is[SA] = 0);
    }
  else if(match("-")) {             /* unary - */
    if(level13(is)) fetch(is);
    gen(ANEG1, 0);
    is[CV] = -is[CV];
    return (is[SA] = 0);
    }
  else if(match("*")) {             /* unary * */
    if(level13(is)) fetch(is);
    if(ptr = is[ST]) is[TI] = ptr[TYPE];
    else             is[TI] = INT;
    is[SA] =       /* no (op 0) stage address */
    is[TA] =       /* not an address */
    is[TC] = 0;    /* not a constant */
    is[CV] = 1;    /* omit fetch() on func call */
    return 1;
    }
  else if(amatch("sizeof", 6)) {    /* sizeof() */
    int sz, p;  char *szptr, sname[NAMESIZE];
    if(match("(")) p = 1;
    else           p = 0;
    sz = 0;
    if     (amatch("unsigned", 8))  sz = BPW;
    if     (amatch("int",      3))  sz = BPW;
    else if(amatch("char",     4))  sz = 1;
    if(sz) {if(match("*"))          sz = BPW;}
    else if(symname(sname)
         && ((szptr = findloc(sname)) ||
             (szptr = findglb(sname)))
         && szptr[IDENT] != FUNCTION
         && szptr[IDENT] != LABEL)    sz = getint(szptr+SIZE, 2);
    else if(sz == 0) error("must be object or type");
    if(p) need(")");
    is[TC] = INT;
    is[CV] = sz;
    is[TA] = is[TI] = is[ST] = 0;
    return 0;
    }
  else if(match("&")) {             /* unary & */
    if(level13(is) == 0) {
      error("illegal address");
      return 0;
      }
    ptr = is[ST];
    is[TA] = ptr[TYPE];
    if(is[TI]) return 0;
    gen(POINT1m, ptr);
    is[TI] = ptr[TYPE];
    return 0;
    }
  else {
    k = level14(is);
    if(match("++")) {               /* lval++ */
      if(k == 0) {
        needlval();
        return 0;
        }
      step(rINC1, is, rDEC1);
      return 0;
      }
    else if(match("--")) {          /* lval-- */
      if(k == 0) {
        needlval();
        return 0;
        }
      step(rDEC1, is, rINC1);
      return 0;
      }
    else return k;
    }
  }

/*
** test primary register against zero and jump if false
*/
zerojump(oper, label, is) int oper, label, is[]; {
  clearstage(is[SA], 0);       /* purge conventional code */
  gen(oper, label);
  }

experr() {
  error("invalid expression");
  gen(GETw1n, 0);
  skip();
  }

store(is)  int is[]; {
  char *ptr;
  if(is[TI]) {                    /* putstk */
    if(is[TI] >> 2 == 1)
         gen(PUTbp1, 0);
    else gen(PUTwp1, 0);
    }
  else {                          /* putmem */
    ptr = is[ST];
    if(ptr[IDENT] != POINTER
    && ptr[TYPE] >> 2 == 1)
         gen(PUTbm1, ptr);
    else gen(PUTwm1, ptr);
    }
  }

