/*
** CC_CODEGEN.C -- resident 8086 output and staging services
**
** The hot staging machinery remains resident because expressions can
** remap every window-A page while a queue is open. The full 8086 code
** template table and interpreter live in CC_CGEN; R_CCOUT and
** R_SETCODES are the only paged calls made here.
**
** M37a restores the original Hendrix module writer: CODE and DATA
** segments, PUBLIC and EXTRN declarations, runtime-helper declarations,
** and the closing END directive. The earlier G:/PUB:/TSEG: diagnostic
** records are retired. Raw bytes still use the compact resident
** pstr/pchar/pdec/pnl primitives in SPYOUT.A99.
*/

	/*
	** Compiler p-code values used by resident gen(). The complete
	** 1..73 template table is owned by CC_CGEN.C.
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

	#define PCODEMAX 74

	#define NAME      7
	#define BPW       2

	/*
	** ==== M32: BASELINE STAGING CAPACITY RESTORED ====
	**
	** 200 p-code entries of (code,value) = 400 ints = 800 bytes,
	** exactly baseline STAGESIZE. The 128-entry reduction (STAGESZ
	** 256) existed only because this module once shared a 4K overlay
	** page; codegen has been RESIDENT since Step 6 and the module
	** sits ~700 bytes under its 4KB spec-2.5 limit, so the original
	** reason is gone.
	**
	** Restored NOW, before self-hosting, deliberately: the M32 build
	** proved the ceiling is real -- the HOST smallcp overflowed its
	** own 200-entry stage on an 18-term && chain -- and Hendrix
	** wrote CCC1-4 assuming 200 entries, so compiling our own
	** sources with 128 was a self-hosting time bomb. Capacity does
	** not change emitted p-codes, so all blessed golden logs remain
	** valid.
	**
	** The overflow bound stays computed inline (stage + STAGESZ);
	** see the module header for why baseline's slast expression is
	** a scaled-pointer bound bug that must NOT come back.
	*/
	#define STAGESZ 400

	extern int csp;

	int stage[STAGESZ];
	int *snext;

	/*
	** M37a: the diagnostic p-code mnemonic table is retired.
	**
	** M36 completed the reachable 8086 template set, so setcodes() now
	** only initialises the real CC_CGEN table. Removing the G: spy data
	** releases resident space for the original Hendrix module-format
	** writer below and makes repeated setcodes() calls safe for SKELTEST.
	*/
	extern R_CCOUT();
	extern R_SETCODES();

	setcodes() {
	  R_SETCODES();
	  }



	/*
	** Generate code into the staging buffer -- baseline CCC4
	** gen(), switch rewritten as if/else (smallcp dialect).
	*/
	gen(pcode, value) int pcode, value; {
	  int newcsp;

	  if(pcode == GETb1pu || pcode == GETb1p ||
	     pcode == GETw1p)
	    gen(MOVE21, 0);
	  else if(pcode == SUB12  || pcode == MOD12 ||
	          pcode == MOD12u || pcode == DIV12 ||
	          pcode == DIV12u)
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
	    R_CCOUT(pcode, value);		/* M35a: emit via codegen overlay */
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

	/*
	** Remember where we are in the queue in case we have to back
	** up. Baseline CCC4 setstage(), verbatim semantics.
	**
	** *before receives the PREVIOUS snext (0 when no stage was
	** open -- that zero is what tells clearstage it owns the
	** queue). *start receives the position to dump from.
	*/
	setstage(before, start) int *before, *start; {
	  if((*before = snext) == 0)
	    snext = stage;
	  *start = snext;
	  }

	/*
	** Dump the contents of the queue.
	**   before != 0 -> an outer stage owns the queue: just rewind
	**   start  == 0 -> throw the contents away
	**   else        -> dump, then close the queue
	** Baseline CCC4 clearstage(), verbatim semantics.
	*/
	clearstage(before, start) int *before, *start; {
	  if(before) {
	    snext = before;
	    return;
	    }
	  if(start)
	    dumpstage();
	  snext = 0;
	  }

	/*
	** Dump the staging buffer. Baseline CCC4 dumpstage() minus the
	** peephole pass: optimize is OFF until self-hosting v2. When
	** peep()/seq[] are ported they slot in here and ONLY here.
	*/
	dumpstage() {
	  int *end;

	  end = snext;
	  snext = stage;

	  while(snext < end) {
	    R_CCOUT(snext[0], snext[1]);		/* M35a: emit via codegen overlay */
	    snext = snext + 2;
	    }
	  }

	/*
	** M37a: assembler module formatting restored from Hendrix CCC4.
	** The output is MASM-compatible 8086 source: CODE/DATA segments,
	** PUBLIC and EXTRN declarations, runtime helper declarations, and
	** a final END directive. oldseg is resident state in CC_DATA.
	*/
	#define IDENT     0
	#define CLASS     2
	#define SYMMAX   16
	#define SYMAVG   12
	#define NUMLOCS  25
	#define NUMGLBS 200
	#define POINTER   3
	#define FUNCTION  4
	#define STATIC    2
	#define AUTOEXT   4
	#define DATASEG   1
	#define CODESEG   2

	#define STARTGLB (symtab + NUMLOCS * SYMAVG)
	#define ENDGLB   (STARTGLB + NUMGLBS * SYMMAX)

	extern char symtab[];
	extern char ssname[];
	extern char *cptr;
	extern int oldseg;
	extern findglb();

	outline(ptr) char *ptr; {
	  pstr(ptr);
	  pnl();
	  }

	outname(ptr) char *ptr; {
	  pchar('_');
	  while(*ptr >= ' ')
	    pchar(*ptr++);
	  }

	outsize(size, ident) int size, ident; {
	  if(size == 1 && ident != POINTER && ident != FUNCTION)
	    pstr("BYTE");
	  else if(ident != FUNCTION)
	    pstr("WORD");
	  else
	    pstr("NEAR");
	  }

	toseg(seg) int seg; {
	  if(oldseg == seg)
	    return;

	  if(oldseg == CODESEG)
	    outline("CODE ENDS");
	  else if(oldseg == DATASEG)
	    outline("DATA ENDS");

	  if(seg == CODESEG) {
	    outline("CODE SEGMENT PUBLIC");
	    outline("ASSUME CS:CODE, SS:DATA, DS:DATA");
	    }
	  else if(seg == DATASEG)
	    outline("DATA SEGMENT PUBLIC");

	  oldseg = seg;
	  }

	public(ident) int ident; {
	  if(ident == FUNCTION)
	    toseg(CODESEG);
	  else
	    toseg(DATASEG);

	  pstr("PUBLIC ");
	  outname(ssname);
	  pnl();
	  outname(ssname);
	  if(ident == FUNCTION) {
	    pchar(':');
	    pnl();
	    }
	  }

	external(name, size, ident) char *name; int size, ident; {
	  if(ident == FUNCTION)
	    toseg(CODESEG);
	  else
	    toseg(DATASEG);

	  pstr("EXTRN ");
	  outname(name);
	  pchar(':');
	  outsize(size, ident);
	  pnl();
	  }

	header() {
	  toseg(CODESEG);
	  outline("extrn __eq: near");
	  outline("extrn __ne: near");
	  outline("extrn __le: near");
	  outline("extrn __lt: near");
	  outline("extrn __ge: near");
	  outline("extrn __gt: near");
	  outline("extrn __ule: near");
	  outline("extrn __ult: near");
	  outline("extrn __uge: near");
	  outline("extrn __ugt: near");
	  outline("extrn __lneg: near");
	  outline("extrn __switch: near");
	  outline("dw 0");
	  toseg(DATASEG);
	  outline("dw 0");
	  }

	trailer() {
	  char *cp;

	  cptr = STARTGLB;
	  while(cptr < ENDGLB) {
	    if(cptr[IDENT] == FUNCTION && cptr[CLASS] == AUTOEXT)
	      external(cptr + NAME, 0, FUNCTION);
	    cptr = cptr + SYMMAX;
	    }

	  cp = findglb("main");
	  if(cp && cp[CLASS] == STATIC)
	    external("_main", 0, FUNCTION);

	  toseg(0);
	  outline("END");
	  }


	/*
	** ==== M30: FIXTURE DELETED ====
	**
	** fixcopy()/fixset() were milestone-27-era scaffolding, marked
	** DELETE when parse() is real. parse() has been real since M29;
	** the harness feeds testsrc directly and nothing referenced the
	** fixture. Removed here because this module must fit its 4KB
	** segment (LINK99 spec 2.5) and dead bytes were part of the
	** overrun.
	*/
