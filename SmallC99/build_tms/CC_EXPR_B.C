#asm
	AORG 9000H
#endasm

/*
** CC_EXPR_B -- CCC3 expression engine, page 2 of a 4-page 16K window
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
** DREL ANCHOR: test(). DREL tags a module by recognising an exported
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
test(label, parens)  int label, parens;  {
  int is[7];
  int *before, *start;
  if(parens) need("(");
  while(1) {
    setstage(&before, &start);
    if(level1(is)) fetch(is);
    if(match(",")) clearstage(before, start);
    else break;
    }
  if(parens) need(")");
  if(is[TC]) {             /* constant expression */
    clearstage(before, 0);
    if(is[CV]) return;
    gen(JMPm, label);
    return;
    }
  if(is[SA]) {             /* stage address of "oper 0" code */
    switch(is[OP]) {       /* operator code */
      case EQ12:
      case LE12u: zerojump(EQ10f, label, is); break;
      case NE12:
      case GT12u: zerojump(NE10f, label, is); break;
      case GT12:  zerojump(GT10f, label, is); break;
      case GE12:  zerojump(GE10f, label, is); break;
      case GE12u: clearstage(is[SA], 0);      break;
      case LT12:  zerojump(LT10f, label, is); break;
      case LT12u: zerojump(JMPm,  label, is); break;
      case LE12:  zerojump(LE10f, label, is); break;
      default:    gen(NE10f, label);          break;
      }
    }
  else gen(NE10f, label);
  clearstage(before, start);
  }

level14(is)  int *is; {
  int k, const, val;
  char *ptr, *before, *start;
  k = primary(is);
  ptr = is[ST];
  blanks();
  if(ch == '[' || ch == '(') {
    int is2[7];                     /* allocate only if needed */
    while(1) {
      if(match("[")) {              /* [subscript] */
        if(ptr == 0) {
          error("can't subscript");
          skip();
          need("]");
          return 0;
          }
        if(is[TA]) {if(k) fetch(is);}
        else       {error("can't subscript"); k = 0;}
        setstage(&before, &start);
        is2[TC] = 0;
        down2(0, 0, level1, is2, is2);
        need("]");
        if(is2[TC]) {
          clearstage(before, 0);
          if(is2[CV]) {             /* only add if non-zero */
            if(ptr[TYPE] >> 2 == BPW)
                 gen(GETw2n, is2[CV] << LBPW);
            else gen(GETw2n, is2[CV]);
            gen(ADD12, 0);
            }
          }
        else {
          if(ptr[TYPE] >> 2 == BPW) gen(DBL1, 0);
          gen(ADD12, 0);
          }
        is[TA] = 0;
        is[TI] = ptr[TYPE];
        k = 1;
        }
      else if(match("(")) {         /* function(...) */
        if(ptr == 0) callfunc(0);
        else if(ptr[IDENT] != FUNCTION) {
          if(k && !is[CV]) fetch(is);
          callfunc(0);
          }
        else callfunc(ptr);
        k = is[ST] = is[TC] = is[CV] = 0;
        }
      else return k;
      }
    }
  if(ptr && ptr[IDENT] == FUNCTION) {
    gen(POINT1m, ptr);
    is[ST] = 0;
    return 0;
    }
  return k;
  }

level2(is1)  int is1[]; {
  int is2[7], is3[7], k, flab, endlab, *before, *after;
  k = down1(level3, is1);                   /* expression 1 */
  if(match("?") == 0) return k;
  dropout(k, NE10f, flab = getlabel(), is1);
  if(down1(level2, is2)) fetch(is2);        /* expression 2 */
  else if(is2[TC]) gen(GETw1n, is2[CV]);
  need(":");
  gen(JMPm, endlab = getlabel());
  gen(LABm, flab);
  if(down1(level2, is3)) fetch(is3);        /* expression 3 */
  else if(is3[TC]) gen(GETw1n, is3[CV]);
  gen(LABm, endlab);

  is1[TC] = is1[CV] = 0;
  if(is2[TC] && is3[TC]) {                  /* expr1 ? const2 : const3 */
    is1[TA] = is1[TI] = is1[SA] = 0;
    }
  else if(is3[TC]) {                        /* expr1 ? var2 : const3 */
    is1[TA] = is2[TA];
    is1[TI] = is2[TI];
    is1[SA] = is2[SA];
    }
  else if((is2[TC])                         /* expr1 ? const2 : var3 */
       || (is2[TA] == is3[TA])) {           /* expr1 ? same2 : same3 */
    is1[TA] = is3[TA];
    is1[TI] = is3[TI];
    is1[SA] = is3[SA];
    }
  else error("mismatched expressions");
  return 0;
  }

step(oper, is, oper2) int oper, is[], oper2; {
  fetch(is);
  gen(oper, is[TA] ? (is[TA] >> 2) : 1);
  store(is);
  if(oper2) gen(oper2, is[TA] ? (is[TA] >> 2) : 1);
  }

/*
** true if is2's operand should be doubl2d
*/
doubl2(oper, is1, is2) int oper, is1[], is2[]; {
  if((oper != ADD12 && oper != SUB12)
  || (is1[TA] >> 2 != BPW)
  || (is2[TA])) return 0;
  return 1;
  }

/*
** test for early dropout from || or && sequences
*/
dropout(k, tcode, exit1, is)
  int k, tcode, exit1, is[]; {
  if(k) fetch(is);
  else if(is[TC]) gen(GETw1n, is[CV]);
  gen(tcode, exit1);          /* jumps on false */
  }
