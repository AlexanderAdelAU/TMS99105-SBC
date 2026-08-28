#asm
	AORG 8000H
#endasm

	/*
	** CC_DECL.C
	**
	** Declaration overlay (OVL_DECL) -- global declarations only.
	** Function-definition half now lives in CC_DFUN.C (OVL_DFUN).
	** Error helpers kill/illname/multidef and needsub moved to
	** CC_RESIDENT.C -- they are called from BOTH halves and a
	** direct same-window cross-page call is never legal.
	**
	** Overlay origin: segment >4000.
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
	#define BYTEr0   12
	#define WORD_    37
	#define WORDr0   39

	#define LITMAX   256
	#define NAMESIZE  9

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
	** Test for global declarations.
	*/
	dodeclare(class) int class; {
	  if     (amatch("char",     4)) declglb(CHR,  class);
	  else if(amatch("unsigned", 8)) {
	    if   (amatch("char",     4)) declglb(UCHR, class);
	    else {
	      amatch("int", 3);
	      declglb(UINT, class);
	      }
	    }
	  else if(amatch("int", 3) || class == EXTERNAL)
	    declglb(INT, class);
	  else
	    return 0;

	  ns();
	  return 1;
	  }

	/*
	** Declare a global/static object.
	*/
	declglb(type, class) int type, class; {
	  int id, dim;

	  while(1) {
	    if(endst()) return;

	    if(match("*")) {
	      id  = POINTER;
	      dim = 0;
	      }
	    else {
	      id  = VARIABLE;
	      dim = 1;
	      }

	    if(symname(ssname) == 0) illname();
	    if(findglb(ssname)) multidef(ssname);

	    if(id == VARIABLE) {
	      if(match("(")) {
	        id = FUNCTION;
	        need(")");
	        }
	      else if(match("[")) {
	        id  = ARRAY;
	        dim = needsub();
	        }
	      }

	    if(class == EXTERNAL)
	      external(ssname, type >> 2, id);
	    else if(id != FUNCTION)
	      initials(type >> 2, id, dim);

	    if(id == POINTER)
	      addsym(ssname, id, type, BPW, 0, &glbptr, class);
	    else
	      addsym(ssname, id, type, dim * (type >> 2),
	             0, &glbptr, class);

	    if(match(",") == 0) return;
	    }
	  }

	/*
	** Initialise a global object.
	*/
	initials(size, ident, dim) int size, ident, dim; {
	  int savedim;

	  litptr = 0;
	  if(dim == 0) dim = -1;
	  savedim = dim;

	  public(ident);

	  if(match("=")) {
	    if(match("{")) {
	      while(dim) {
	        init(size, ident, &dim);
	        if(match(",") == 0) break;
	        }
	      need("}");
	      }
	    else
	      init(size, ident, &dim);
	    }

	  if(savedim == -1 && dim == -1) {
	    if(ident == ARRAY) error("need array size");
	    dstowlit(0, size = BPW);
	    }

	  ddumplit(size);
	  ddumpzer(size, dim);
	  if((litptr + (dim > 0 ? dim * size : 0)) & 1) {
	    pstr("\tEVEN");   /* TMS: realign after an odd */
	    pnl();            /* sized global object       */
	    }
	  }

	/*
	** Evaluate one initializer.
	*/
	init(size, ident, dim) int size, ident, *dim; {
	  int value;

	  if(string(&value)) {
	    if(ident == VARIABLE || size != 1)
	      error("must assign to char pointer or char array");

	    *dim -= litptr - value;

	    if(ident == POINTER)
	      dpoint();
	    }
	  else if(constexpr(&value)) {
	    if(ident == POINTER)
	      error("cannot assign to pointer");

	    dstowlit(value, size);
	    *dim -= 1;
	    }
	  }

	/*
	** M36b4 data-output helpers.
	**
	** litq[] is resident static storage. These routines live with the
	** declaration grammar because all global initializers call them and
	** this overlay has ample page headroom. They are the baseline CCC3/
	** CCC4 algorithms with resident output primitives substituted for
	** stdio. The function-literal pool has its own copy in CC_DFUN.C so
	** no same-window cross-overlay call is introduced.
	*/
	dstowlit(value, size) int value, size; {
	  if((litptr + size) >= LITMAX) {
	    error("literal queue overflow");
	    return;
	    }
	  putint(value, litq + litptr, size);
	  litptr += size;
	  }

	ddumplit(size) int size; {
	  int j, k;

	  k = 0;
	  while(k < litptr) {
	    if(size == 1)
	         gen(BYTE_, 0);
	    else gen(WORD_, 0);

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

	ddumpzer(size, count) int size, count; {
	  if(count > 0) {
	    if(size == 1)
	         gen(BYTEr0, count);
	    else gen(WORDr0, count * BPW);
	    }
	  }

	dpoint() {
	  pstr(" DW $+2");
	  pnl();
	  }
