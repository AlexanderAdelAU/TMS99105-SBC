/*
** CC1.C - real Small-C top-level dispatch chain, ported from the
** actual uploaded source. Kept entirely RESIDENT for now (no
** overlay placement) -- that's a separate decision, not made here.
**
** REAL, verbatim (adapted only where a dependency below is
** stubbed): parse(), dodeclare(), declglb(), initials(), init(),
** needsub(), dofunction(), doargs(), decl().
**
** main(): NOT verbatim. ask() removed entirely (argc/argv, command-
** line options -- no equivalent in this environment). calloc()-based
** buffer setup removed (no allocator ported). header()/setcodes()/
** trailer()/fclose() removed (codegen-side, CC4 not ported). What's
** left: call parse() directly, assuming the test harness primes
** @line the same way every other test in this project already does.
**
** STUBBED, honestly, not silently: everything below is a genuine
** gap, not a design choice -- each just returns a safe placeholder
** so the real dispatch chain above it can run without crashing.
**   - addsym/findglb/findloc: symbol table not ported (needs real
**     buffer sizes from cc.h). findglb/findloc always report "not
**     found"; addsym returns a fixed dummy address.
**   - constexpr/string: expression evaluation, lives in CC3/EXPR
**     overlay, not reachable from resident C yet. constexpr always
**     reports "not a constant" (0); string always reports "not a
**     string" (0).
**   - gen/toseg/external/public/stowlit/dumplits/dumpzero/point:
**     code generation, CC4 not ported. All no-ops.
**   - statement(): the real statement dispatcher (doif/dowhile/
**     etc.) isn't wired up yet either -- stub skips to the matching
**     closing brace so dofunction() can complete without crashing,
**     not real parsing.
**
** Needs CC2.C's real, already-proven functions: gch, bump, blanks,
** match, streq, astreq, amatch, symname, need, ns, endst, hash,
** getlabel, getint, putint, error, skip, alpha, an, isalpha,
** isdigit, ch, nch, line, lptr, eof, mline, errflag, nxtlab.
*/

extern char *line, *lptr, *mline;
extern int ch, nch, eof, errflag, nxtlab;

#define CHR 1
#define INT 2
#define UCHR 3
#define UINT 4
#define POINTER 1
#define VARIABLE 2
#define FUNCTION 3
#define ARRAY 4
#define EXTERNAL 1
#define STATIC 2
#define AUTOMATIC 3
#define AUTOEXT 4
#define BPW 2
#define NAMESIZE 9

char ssname[NAMESIZE];
int argstk, argtop, csp, litptr, litlab;
int nogo, noloc, lastst;
int monitor;

/*
** -------- stubs: symbol table (not ported -- needs cc.h buffer
** sizes to allocate real symtab/glbptr/locptr storage) --------
*/


findglb(sname) char *sname; {
  return 0;
  }

findloc(sname) char *sname; {
  return 0;
  }

addsym(sname, id, type, size, value, lgpp, class)
  char *sname, id, type; int size, value, *lgpp, class; {
  return 1;  /* dummy nonzero "address" so callers don't null-check-fail */
  }

/*
** -------- stubs: expression evaluation (lives in the EXPR
** overlay, not reachable from resident C yet) --------
*/
constexpr(val) int *val; {
  return 0;
  }

string(val) int *val; {
  return 0;
  }

/*
** -------- stubs: code generation (CC4, not ported) --------
*/
gen(op, arg) int op, arg; {
  }

toseg(seg) int seg; {
  }

external(sname, size, id) char *sname; int size, id; {
  }

public(ident) int ident; {
  }

stowlit(value, size) int value, size; {
  }

dumplits(size) int size; {
  }

dumpzero(size, dim) int size, dim; {
  }

point() {
  }

/*
** -------- stub: statement() -- real dispatcher not wired up yet.
** Skips to the matching closing brace (if the body is a compound
** statement) or a single token (if not), just enough for
** dofunction() to complete without crashing. NOT real parsing.
*/
statement() {
  int depth;
  if (match("{") == 0) {
    skip();
    return;
    }
  depth = 1;
  while (depth > 0 && eof == 0) {
    if (match("{")) { depth = depth + 1; continue; }
    if (match("}")) { depth = depth - 1; continue; }
    if (ch == 0) break;
    gch();
    }
  }

/*
** ================= REAL, from the actual source =================
*/

/*
** process all input text
*/

parse() {

  while (eof == 0) {
    if     (amatch("extern", 6)) dodeclare(EXTERNAL);
    else if(dodeclare(STATIC))   ;
    else if( match("#asm"))      skip();       /* doasm() not ported */
    else if( match("#include"))  skip();       /* doinclude() not ported */
    else if( match("#define"))   skip();       /* dodefine() not ported */
    else                         dofunction();
    blanks();
    }
  }

/*
** test for global declarations
*/
dodeclare(class) int class; {
  if     (amatch("char",     4))  declglb(CHR,  class);
  else if(amatch("unsigned", 8)) {
    if   (amatch("char",     4))  declglb(UCHR, class);
    else {amatch("int",      3);  declglb(UINT, class);}
    }
  else if(amatch("int",      3)
       || class == EXTERNAL)      declglb(INT,  class);
  else return 0;
  ns();
  return 1;
  }

/*
** declare a static variable
*/
declglb(type, class)  int type, class; {
  int id, dim;
  while(1) {
    if(endst()) return;
    if(match("*"))       {id = POINTER;  dim = 0;}
    else                 {id = VARIABLE; dim = 1;}
    if(symname(ssname) == 0) illname();
    if(findglb(ssname)) multidef(ssname);
    if(id == VARIABLE) {
      if     (match("("))  {id = FUNCTION; need(")");}
      else if(match("["))  {id = ARRAY; dim = needsub();}
      }
    if     (class == EXTERNAL) external(ssname, type >> 2, id);
    else if(   id != FUNCTION) initials(type >> 2, id, dim);
    if(id == POINTER)
         addsym(ssname, id, type, BPW, 0, 0, class);
    else addsym(ssname, id, type, dim * (type >> 2), 0, 0, class);
    if(match(",") == 0) return;
    }
  }

/*
** initialize global objects
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
    else init(size, ident, &dim);
    }
  if(savedim == -1 && dim == -1) {
    if(ident == ARRAY) error("need array size");
    stowlit(0, size = BPW);
    }
  dumplits(size);
  dumpzero(size, dim);
  }

/*
** evaluate one initializer
*/
init(size, ident, dim) int size, ident, *dim; {
  int value;
  if(string(&value)) {
    if(ident == VARIABLE || size != 1)
      error("must assign to char pointer or char array");
    *dim -= (litptr - value);
    if(ident == POINTER) point();
    }
  else if(constexpr(&value)) {
    if(ident == POINTER) error("cannot assign to pointer");
    stowlit(value, size);
    *dim -= 1;
    }
  }

/*
** get required array size
*/
needsub()  {
  int val;
  if(match("]")) return 0;
  if(constexpr(&val) == 0) val = 1;
  if(val < 0) {
    error("negative size illegal");
    val = -val;
    }
  need("]");
  return val;
  }

/*
** function definition
*/
dofunction()  {
  char *ptr;
  nogo   =
  noloc  =
  lastst =
  litptr = 0;
  litlab = getlabel();
  if(match("void")) blanks();
  if(symname(ssname) == 0) {
    error("illegal function or declaration");
    errflag = 0;
    kill();
    return;
    }
  if(ptr = findglb(ssname)) {
    if(ptr == AUTOEXT)
         ptr = STATIC;
    else multidef(ssname);
    }
  else addsym(ssname, FUNCTION, INT, 0, 0, 0, STATIC);
  public(FUNCTION);
  argstk = 0;
  if(match("(") == 0) error("no open paren");
  while(match(")") == 0) {
    if(symname(ssname)) {
      if(findloc(ssname)) multidef(ssname);
      else {
        addsym(ssname, 0, 0, 0, argstk, 0, AUTOMATIC);
        argstk += BPW;
        }
      }
    else {
      error("illegal argument name");
      skip();
      }
    blanks();
    if(streq(lptr,")") == 0 && match(",") == 0)
      error("no comma");
    if(endst()) break;
    }
  csp = 0;
  argtop = argstk+BPW;
  while(argstk) {
    if     (amatch("char",     4)) {doargs(CHR);  ns();}
    else if(amatch("int",      3)) {doargs(INT);  ns();}
    else if(amatch("unsigned", 8)) {
      if   (amatch("char", 4))     {doargs(UCHR); ns();}
      else {amatch("int", 3);       doargs(UINT); ns();}
      }
    else {error("wrong number of arguments"); break;}
    }
  gen(1, 0);
  statement();
  if(lastst != 0)
    gen(2, 0);
  if(litptr) {
    toseg(1);
    gen(3, litlab);
    dumplits(1);
    }
  }

/*
** declare argument types
*/
doargs(type) int type; {
  int id, sz;
  char c, *ptr;
  while(1) {
    if(argstk == 0) return;
    if(decl(type, POINTER, &id, &sz)) {
      if(ptr = findloc(ssname)) {
        /* symbol table not ported: nothing to patch */
        }
      else error("not an argument");
      }
    argstk = argstk - BPW;
    if(endst()) return;
    if(match(",") == 0) error("no comma");
    }
  }

/*
** parse next local or argument declaration
*/
decl(type, aid, id, sz) int type, aid, *id, *sz; {
  int n, p;
  if(match("(")) p = 1;
  else           p = 0;
  if(match("*"))        {*id = POINTER;  *sz  = BPW;}
  else                  {*id = VARIABLE; *sz  = type >> 2;}
  if((n = symname(ssname)) == 0) illname();
  if(p && match(")")) ;
  if(match("(")) {
    if(!p || *id != POINTER) error("try (*...)()");
    need(")");
    }
  else if(*id == VARIABLE && match("[")) {
    *id = aid;
    if((*sz *= needsub()) == 0) {
      if(aid == ARRAY) error("need array size");
      *sz  = BPW;
      }
    }
  return n;
  }

/*
** kill() is really CC2.C content (confirmed in the real source),
** not CC1.C -- placed here temporarily since dofunction() needs it
** and CC2.C doesn't have it yet. Move it when CC2.C's own
** functions catch up to this point in the real file.
*/
kill() {
  *line = 0;
  bump(0);
  }

illname() {
  error("illegal symbol");
  skip();
  }

multidef(sname)  char *sname; {
  error("already defined");
  }

