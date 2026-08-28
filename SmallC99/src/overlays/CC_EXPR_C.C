#asm
	AORG 0A000H
#endasm

/*
** CC_EXPR_C -- CCC3 expression engine, page 3 of a 4-page 16K window
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
** DREL ANCHOR: level1(). DREL tags a module by recognising an exported
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
/***************** precedence levels ******************/

level1(is) int is[];  {
  int k, is2[7], is3[2], oper, oper2;
  k = down1(level2, is);
  if(is[TC]) gen(GETw1n, is[CV]);
       if(match("|="))  {oper =        oper2 = OR12;}
  else if(match("^="))  {oper =        oper2 = XOR12;}
  else if(match("&="))  {oper =        oper2 = AND12;}
  else if(match("+="))  {oper =        oper2 = ADD12;}
  else if(match("-="))  {oper =        oper2 = SUB12;}
  else if(match("*="))  {oper = MUL12; oper2 = MUL12u;}
  else if(match("/="))  {oper = DIV12; oper2 = DIV12u;}
  else if(match("%="))  {oper = MOD12; oper2 = MOD12u;}
  else if(match(">>=")) {oper =        oper2 = ASR12;}
  else if(match("<<=")) {oper =        oper2 = ASL12;}
  else if(match("="))   {oper =        oper2 = 0;}
  else return k;
                        /* have an assignment operator */
  if(k == 0) {
    needlval();
    return 0;
    }
  is3[ST] = is[ST];
  is3[TI] = is[TI];
  if(is[TI]) {                             /* indirect target */
    if(oper) {                             /* ?= */
      gen(PUSH1, 0);                       /* save address */
      fetch(is);                           /* fetch left side */
      }
    down2(oper, oper2, level1, is, is2);   /* parse right side */
    if(oper) gen(POP2, 0);                 /* retrieve address */
    }
  else {                                   /* direct target */
    if(oper) {                             /* ?= */
      fetch(is);                           /* fetch left side */
      down2(oper, oper2, level1, is, is2); /* parse right side */
      }
    else {                                 /*  = */
      if(level1(is2)) fetch(is2);          /* parse right side */
      }
    }
  store(is3);                              /* store result */
  return 0;
  }

/*
** binary drop to a lower level
*/
down2(oper, oper2, level, is, is2)
  int oper, oper2, (*level)(), is[], is2[]; {
  int *before, *start;
  char *ptr;
  setstage(&before, &start);
  is[SA] = 0;                     /* not "... op 0" syntax */
  if(is[TC]) {                    /* consant op unknown */
    if(down1(level, is2)) fetch(is2);
    if(is[CV] == 0) is[SA] = snext;
    gen(GETw2n, is[CV] << doubl2(oper, is2, is));
    }
  else {                          /* variable op unknown */
    gen(PUSH1, 0);                /* at start in the buffer */
    if(down1(level, is2)) fetch(is2);
    if(is2[TC]) {                 /* variable op constant */
      if(is2[CV] == 0) is[SA] = start;
      csp += BPW;                 /* adjust stack and */
      clearstage(before, 0);      /* discard the PUSH */
      if(oper == ADD12) {         /* commutative */
        gen(GETw2n, is2[CV] << doubl2(oper, is, is2));
        }
      else {                      /* non-commutative */
        gen(MOVE21, 0);
        gen(GETw1n, is2[CV] << doubl2(oper, is, is2));
        }
      }
    else {                        /* variable op variable */
      gen(POP2, 0);
      if(doubl2(oper, is, is2)) gen(DBL1, 0);
      if(doubl2(oper, is2, is)) gen(DBL2, 0);
      }
    }
  if(oper) {
    if(nosign(is) || nosign(is2)) oper = oper2;
    if(is[TC] = is[TC] & is2[TC]) {               /* constant result */
      is[CV] = calc(is[CV], oper, is2[CV]);
      clearstage(before, 0);
      if(is2[TC] == UINT) is[TC] = UINT;
      }
    else {                                        /* variable result */
      gen(oper, 0);
      if(oper == SUB12
      && is [TA] >> 2 == BPW
      && is2[TA] >> 2 == BPW) { /* difference of two word addresses */
        gen(SWAP12, 0);
        gen(GETw1n, 1);
        gen(ASR12, 0);          /* div by 2 */
        }
      is[OP] = oper;            /* identify the operator */
      }
    if(oper == SUB12 || oper == ADD12) {
      if(is[TA] && is2[TA])     /*  addr +/- addr */
        is[TA] = 0;
      else if(is2[TA]) {        /* value +/- addr */
        is[ST] = is2[ST];
        is[TI] = is2[TI];
        is[TA] = is2[TA];
        }
      }
    if(is[ST] == 0 || ((ptr = is2[ST]) && (ptr[TYPE] & UNSIGNED)))
      is[ST] = is2[ST];
    }
  }

/*
** drop to a lower level
*/
down(opstr, opoff, level, is)
  char *opstr;  int opoff, (*level)(), is[]; {
  int k;
  k = down1(level, is);
  if(nextop(opstr) == 0) return k;
  if(k) fetch(is);
  while(1) {
    if(nextop(opstr)) {
      int is2[7];     /* allocate only if needed */
      bump(opsize);
      opindex += opoff;
      down2(op[opindex], op2[opindex], level, is, is2);
      }
    else return 0;
    }
  }

/*
** unary drop to a lower level
*/
down1(level, is) int (*level)(), is[]; {
  int k, *before, *start;
  setstage(&before, &start);
  k = (*level)(is);
  if(is[TC]) clearstage(before, 0);  /* load constant later */
  return k;
  }

/*
** unsigned operand?
*/
nosign(is) int is[]; {
  char *ptr;
  if(is[TA]
  || is[TC] == UINT
  || ((ptr = is[ST]) && (ptr[TYPE] & UNSIGNED))
    ) return 1;
  return 0;
  }

/*
** calcualte unsigned constant result
*/
calc2(left, oper, right) unsigned left, right; int oper; {
  switch(oper) {
    case MUL12u: return (left  *  right);
    case DIV12u: return (left  /  right);
    case MOD12u: return (left  %  right);
    case LE12u:  return (left  <= right);
    case GE12u:  return (left  >= right);
    case LT12u:  return (left  <  right);
    case GT12u:  return (left  >  right);
    }
  return (0);
  }

