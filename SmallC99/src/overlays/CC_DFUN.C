#asm
	AORG 8000H
#endasm

	/*
	** CC_DFUN.C
	**
	** Function-definition overlay (OVL_DFUN) -- split from CC_DECL.C
	** along the grammar seam: global declarations stay in CC_DECL.C
	** (OVL_DECL); function definitions live here.
	**
	** kill/illname/multidef/needsub are resident (CC_RESIDENT.C) --
	** shared with the declaration half; same-window cross-page calls
	** are never legal, so shared helpers live below the window.
	**
	** Overlay origin: segment >4000. Same window as OVL_DECL --
	** NEVER add a direct call to anything in CC_DECL.C from here:
	** the linker will resolve it silently and it will jump into
	** this module's own bytes at runtime.
	*/

	#define BPW       2

	#define IDENT     0
	#define TYPE      1
	#define CLASS     2
	#define SIZE      3
	#define OFFSET    5

	#define LABEL     0
	#define VARIABLE  1
	#define ARRAY     2
	#define POINTER   3
	#define FUNCTION  4

	#define CHR       4
	#define INT       8
	#define UCHR      5
	#define UINT      9

	#define AUTOMATIC 1
	#define STATIC    2
	#define EXTERNAL  3
	#define AUTOEXT   4

	#define DATASEG   1

	#define BYTE_    10

	#define STRETURN  3
	#define STGOTO   13

	#define ENTER    18
	#define REFm     66
	#define RETURN   67

	#define NAMESIZE  9
	#define STARTLOC  symtab

	extern char symtab[];
	extern char litq[];

	extern char
	  *glbptr,
	  *locptr,
	  *line,
	  *lptr,
	   ssname[NAMESIZE];

	extern int
	  argstk,
	  argtop,
	  csp,
	  errflag,
	  lastst,
	  litlab,
	  litptr,
	  nogo,
	  noloc,
	  eof,
	  monitor;

	/*
	** Begin a function definition.
	*/
	dofunction() {
	  char *ptr;

	  nogo   =
	  noloc  =
	  lastst =
	  litptr = 0;

	  litlab = getlabel();
	  locptr = STARTLOC;

	  if(match("void"))
	    blanks();

	  if(monitor)
	    lout(line, 2);

	  if(symname(ssname) == 0) {
	    error("illegal function or declaration");
	    errflag = 0;
	    kill();
	    return;
	    }

	  if(ptr = findglb(ssname)) {
	    if(ptr[CLASS] == AUTOEXT)
	      ptr[CLASS] = STATIC;
	    else
	      multidef(ssname);
	    }
	  else
	    addsym(ssname, FUNCTION, INT, 0, 0, &glbptr, STATIC);

	  public(FUNCTION);

	  argstk = 0;

	  if(match("(") == 0)
	    error("no open paren");

	  while(match(")") == 0) {
	    if(symname(ssname)) {
	      if(findloc(ssname))
	        multidef(ssname);
	      else {
	        addsym(ssname, 0, 0, 0, argstk,
	               &locptr, AUTOMATIC);
	        argstk += BPW;
	        }
	      }
	    else {
	      error("illegal argument name");
	      skip();
	      }

	    blanks();

	    if(streq(lptr, ")") == 0 && match(",") == 0)
	      error("no comma");

	    if(endst()) break;
	    }

	  csp    = 0;
	  argtop = argstk + BPW;

	  while(argstk) {
	    if(amatch("char", 4)) {
	      doargs(CHR);
	      ns();
	      }
	    else if(amatch("int", 3)) {
	      doargs(INT);
	      ns();
	      }
	    else if(amatch("unsigned", 8)) {
	      if(amatch("char", 4))
	        doargs(UCHR);
	      else {
	        amatch("int", 3);
	        doargs(UINT);
	        }
	      ns();
	      }
	    else {
	      error("wrong number of arguments");
	      break;
	      }
	    }

	  gen(ENTER, 0);
	  statement();

	  if(lastst != STRETURN && lastst != STGOTO)
	    gen(RETURN, 0);

	  if(litptr) {
	    toseg(DATASEG);
	    gen(REFm, litlab);
	    fdumplit(1);
	    }
	  }

	/*
	** Declare argument types.
	*/
	doargs(type) int type; {
	  int id, sz;
	  char *ptr;

	  while(1) {
	    if(argstk == 0) return;

	    if(decl(type, POINTER, &id, &sz)) {
	      if(ptr = findloc(ssname)) {
	        ptr[IDENT] = id;
	        ptr[TYPE]  = type;
	        putint(sz, ptr + SIZE, 2);
	        putint(argtop - getint(ptr + OFFSET, 2),
	               ptr + OFFSET, 2);
	        }
	      else
	        error("not an argument");
	      }

	    argstk -= BPW;

	    if(endst()) return;
	    if(match(",") == 0)
	      error("no comma");
	    }
	  }

	/*
	** ==== M30: decl() MOVED to CC_RESIDENT.C ====
	**
	** It is now called from doargs() here (OVL_DFUN) and from
	** declloc() in CC_STMT (OVL_STMT) -- two pages of the SAME
	** window, so it must live below the window. Same precedent as
	** kill/illname/multidef/needsub. The call above resolves as a
	** clean EXT; do NOT add "extern decl();" (see the CC_EXPR
	** header for why that binds to a local stub instead).
	*/

	/*
	** M36b4 function literal-pool dumper.
	**
	** Function strings share the resident litq[] queue, but the dump is
	** kept in this overlay to avoid a same-window call into CC_DECL.
	*/
	fdumplit(size) int size; {
	  int j, k;

	  k = 0;
	  while(k < litptr) {
	    gen(BYTE_, 0);
	    j = 10;
	    while(j--) {
	      pdec(getint(litq + k, size));
	      k += size;
	      if(j == 0 || k >= litptr) {
	        pnl();
	        break;
	        }
	      pchar(',');
	      }
	    }
	  }
