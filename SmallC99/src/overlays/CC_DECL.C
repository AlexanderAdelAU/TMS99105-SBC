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
	#define TYPEDEFNAME 6
	#define TYPEDEFDECL 5

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
	#define NAMESIZE  16

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
	** struct|union: the parser is its own overlay (CC_STRD); reach it
	** through the framed trampoline R_STRUCT(isunion, outer).
	*/
	#define STRUCTBIT 2
	/*
	** Storage for a global struct object (or array of them): size bytes,
	** zero-filled.  Struct initialisers are phase 2.
	*/
	dstrobj(ident, size) int ident, size; {
	  public(ident);
	  if(match("=")) error("no struct initialiser yet");
	  gen(BYTEr0, size);
	  }

	/*
	** Test for global declarations.
	*/
	dodeclare(class) int class; {
	  int t, baseid, basesz, deftype;
	  deftype = amatch("typedef", 7);
	  if(typeintro(&t, &baseid, &basesz, 0) == 0) {
	    if(deftype) error("missing typedef type");
	    return deftype;
	    }
	  declglb(t, deftype ? TYPEDEFDECL : class, baseid, basesz);
	  ns();
	  return 1;
	  }

	/*
	** Declare a global/static object.
	*/
	declglb(type, class, baseid, basesz) int type, class, baseid, basesz; {
	  int id, dim, t, nstars;

	  while(1) {
	    if(endst()) return;
	    t = type;                 /* per declarator: char *a, b; */

	    id = baseid;
	    dim = 1;
	    if(baseid == POINTER) dim = 0;
	    if(baseid == ARRAY) dim = basesz / elsize(t);
	    if(match("*")) {
	      id = POINTER;
	      dim = 0;
	      while(match("*")) t = t + 64;
	      if(baseid == POINTER) t = t + 64;
	      if(baseid == ARRAY) error("pointer to typedef array unsupported");
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
	        t   = R_STRUCT(2, t);   /* further [..]: rows (CC_STRD) */
	        }
	      }
	    else if(id == POINTER && match("[")) {
	      t   = t + 64;     /* array of pointer alias or declarator */
	      id  = ARRAY;
	      dim = needsub();
	      t   = R_STRUCT(2, t);
	      }
	    else if(id == ARRAY && match("[")) {
	      error("array of typedef array unsupported");
	      dim = needsub();
	      t = R_STRUCT(2, t);
	      }
	    if(t > 255) error("too many *");

	    if(class == TYPEDEFDECL)
	      ;                    /* no object storage for type aliases */
	    else if(class == EXTERNAL)
	      external(ssname, elsize(t), id);
	    else if(id == FUNCTION)
	      ;
	    else if((t & 0xC2) == STRUCTBIT && id != POINTER)
	      dstrobj(id, dim * elsize(t));
	    else if(t & STRUCTBIT)
	      initials(BPW, id, dim);
	    else
	      initials(elsize(t), id, dim);

	    if(class == TYPEDEFDECL)
	      addsym(ssname, TYPEDEFNAME, t,
	             id == POINTER ? BPW : dim * elsize(t),
	             id, &glbptr, STATIC);
	    else if(id == POINTER)
	      addsym(ssname, id, t, BPW, 0, &glbptr, class);
	    else
	      addsym(ssname, id, t, dim * elsize(t),
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
	  pstr(" WORD $+2");
	  pnl();
	  }
