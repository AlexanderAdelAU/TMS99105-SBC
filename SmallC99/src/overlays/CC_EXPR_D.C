#asm
	AORG 0B000H
#endasm

/*
** CC_EXPR_D -- CCC3 expression engine, page 4 of a 4-page 16K window
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
** DREL ANCHOR: primary(). DREL tags a module by recognising an exported
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
primary(is)  int *is; {
  char *ptr, sname[NAMESIZE];
  int k;
  if(match("(")) {                  /* (subexpression) */
    do k = level1(is); while(match(","));
    need(")");
    return k;
    }
  putint(0, is, 7 << LBPW);         /* clear "is" array */
  if(symname(sname)) {              /* is legal symbol */
    if(ptr = findloc(sname)) {      /* is local */
      if(ptr[IDENT] == LABEL) {
        experr();
        return 0;
        }
      k = getint(ptr+OFFSET, 2);
      if(ptr[IDENT] == VARIABLE && (ptr[TYPE] >> 2) == 1)
        ++k;                /* TMS: a scalar char lives in the LSB */
                            /* of its word (cc6 getloc +1); aim     */
                            /* POINT1s at the odd byte. Arrays and   */
                            /* pointers get NO shift.                */
      gen(POINT1s, k);
      is[ST] = ptr;
      is[TI] = ptr[TYPE];
      if(ptr[IDENT] == ARRAY) {
        is[TA] = ptr[TYPE];
        return 0;
        }
      if(ptr[IDENT] == POINTER) {
        is[TI] = UINT;
        is[TA] = ptr[TYPE];
        }
      return 1;
      }
    if(ptr = findglb(sname)) {      /* is global */
      is[ST] = ptr;
      if(ptr[IDENT] != FUNCTION) {
        if(ptr[IDENT] == ARRAY) {
          gen(POINT1m, ptr);
          is[TI] =
          is[TA] = ptr[TYPE];
          return 0;
          }
        if(ptr[IDENT] == POINTER)
          is[TA] = ptr[TYPE];
        return 1;
        }
      }
    else is[ST] = addsym(sname, FUNCTION, INT, 0, 0, &glbptr, AUTOEXT);
    return 0;
    }
  if(constant(is) == 0) experr();
  return 0;
  }

/*
** skim over terms adjoining || and && operators
*/
skim(opstr, tcode, dropval, endval, level, is)
  char *opstr;
  int tcode, dropval, endval, (*level)(), is[]; {
  int k, droplab, endlab;
  droplab = 0;
  while(1) {
    k = down1(level, is);
    if(nextop(opstr)) {
      bump(opsize);
      if(droplab == 0) droplab = getlabel();
      dropout(k, tcode, droplab, is);
      }
    else if(droplab) {
      dropout(k, tcode, droplab, is);
      gen(GETw1n, endval);
      gen(JMPm, endlab = getlabel());
      gen(LABm, droplab);
      gen(GETw1n, dropval);
      gen(LABm, endlab);
      is[TI] = is[TA] = is[TC] = is[CV] = is[SA] = 0;
      return 0;
      }
    else return k;
    }
  }

callfunc(ptr)  char *ptr; {      /* symbol table entry or 0 */
  int nargs, const, val;
  nargs = 0;
  blanks();                      /* already saw open paren */
  while(streq(lptr, ")") == 0) {
    if(endst()) break;
    if(ptr) {
      expression(&const, &val);
      gen(PUSH1, 0);
      }
    else {
      gen(PUSH1, 0);
      expression(&const, &val);
      gen(SWAP1s, 0);            /* don't push addr */
      }
    nargs = nargs + BPW;         /* count args*BPW */
    if(match(",") == 0) break;
    }
  need(")");
  if(streq(ptr + NAME, "CCARGC") == 0) gen(ARGCNTn, nargs >> LBPW);
  if(ptr) gen(CALLm, ptr);
  else    gen(CALL1, 0);
  gen(ADDSP, csp + nargs);
  }

/*
** calcualte signed constant result
*/
calc(left, oper, right) int left, oper, right; {
  switch(oper) {
    case ADD12: return (left  +  right);
    case SUB12: return (left  -  right);
    case MUL12: return (left  *  right);
    case DIV12: return (left  /  right);
    case MOD12: return (left  %  right);
    case EQ12:  return (left  == right);
    case NE12:  return (left  != right);
    case LE12:  return (left  <= right);
    case GE12:  return (left  >= right);
    case LT12:  return (left  <  right);
    case GT12:  return (left  >  right);
    case AND12: return (left  &  right);
    case OR12:  return (left  |  right);
    case XOR12: return (left  ^  right);
    case ASR12: return (left  >> right);
    case ASL12: return (left  << right);
    }
  return (calc2(left, oper, right));
  }

fetch(is) int is[]; {
  char *ptr;
  ptr = is[ST];
  if(is[TI]) {                                   /* indirect */
    if(is[TI] >> 2 == BPW)     gen(GETw1p,  0);
    else {
      if(ptr[TYPE] & UNSIGNED) gen(GETb1pu, 0);
      else                     gen(GETb1p,  0);
      }
    }
  else {                                         /* direct */
    if(ptr[IDENT] == POINTER
    || ptr[TYPE] >> 2 == BPW)  gen(GETw1m,  ptr);
    else {
      if(ptr[TYPE] & UNSIGNED) gen(GETb1mu, ptr);
      else                     gen(GETb1m,  ptr);
      }
    }
  }

level3 (is) int is[]; {return skim("||", EQ10f, 1, 0, level4,  is);}
level4 (is) int is[]; {return skim("&&", NE10f, 0, 1, level5,  is);}
level5 (is) int is[]; {return down("|",            0, level6,  is);}
level6 (is) int is[]; {return down("^",            1, level7,  is);}
level7 (is) int is[]; {return down("&",            2, level8,  is);}
level8 (is) int is[]; {return down("== !=",        3, level9,  is);}
level9 (is) int is[]; {return down("<= >= < >",    5, level10, is);}
level10(is) int is[]; {return down(">> <<",        9, level11, is);}
level11(is) int is[]; {return down("+ -",         11, level12, is);}
level12(is) int is[]; {return down("* / %",       13, level13, is);}
