/*
** CC_RESIDENT.C
**
** Resident compiler core and overlay-facing dispatch.
**
** parse() never calls a window-A implementation directly. R_DODECL
** and R_DOFN are resident assembly wrappers in OVLSTUBS.A99 which
** map the required page and call the generated fixed entry address.
**
** statement(), constexpr() and expression() remain resident symbols
** because existing overlay C sources call those names. They are now
** real bridge shims rather than empty placeholders.
*/

/* ---- Step 4c: fields of the engine's is[7] lvalue array ---- */
#define TC       3      /* type of constant (INT or UINT), else 0 */
#define CV       4      /* value of constant                      */

/* ---- p-codes used by constant() ---- */
#define POINT1l 24
#define GETw1n  31

#define EXTERNAL 3
#define STATIC   2
#define INT      8
#define UINT     9

/* ---- M30: needed by decl(), moved here from CC_DFUN ---- */
#define BPW      2
#define VARIABLE 1
#define ARRAY    2
#define POINTER  3

#define SYMAVG  12
#define NUMLOCS 25
#define SYMTBSZ 3500
#define LITMAX  255

#define WQTABSZ 30
#define WQSIZ    3
#define WQSP     0
#define WQLOOP   1
#define WQEXIT   2
#define WQMAX   (wq + WQTABSZ - WQSIZ)

int argstk;
int argtop;
int csp;
int lastst;
int litlab;
int litptr;
int nogo;
int noloc;

/* Resident because CC_STMT is remapped during expression calls. */
int wq[WQTABSZ];
int *wqptr;

extern char symtab[];
extern char litq[];
extern char *glbptr;
extern char *locptr;
extern char *cptr;
extern char *cptr2;
extern char *cptr3;

char ssname[9];
int monitor;

extern char *line;
extern int eof;
extern int errflag;
extern int nxtlab;
extern int usexpr;
extern int declared;
extern int ncmp;
extern int swactive;
extern int swdefault;
extern int ccode;
extern int pptr;
extern int macptr;
extern int iflevel;
extern int skiplevel;
extern int incunit;
extern int oldseg;
extern int *swnext;
extern int *swend;
extern int swstab[];
extern int ch;
extern int nch;

/* Resident assembly entry points exported by OVLSTUBS.A99. */
extern R_DODECL();
extern R_DOFN();
extern R_STMT();
extern R_TEST();
extern R_CEXPR();
extern R_EXPR();
extern R_EXPRVAL();
extern R_PREP();
extern R_DEFINE();
extern R_INCLUDE();
extern R_DOASM();

ccinit()
{
    int i;

    i = 0;
    while(i < SYMTBSZ)
        symtab[i++] = 0;

    locptr = symtab;
    glbptr = symtab + NUMLOCS * SYMAVG;

    cptr  = 0;
    cptr2 = 0;
    cptr3 = 0;

    argstk = 0;
    argtop = 0;
    csp = 0;
    lastst = 0;
    litlab = 0;
    litptr = 0;
    nogo = 0;
    noloc = 0;
    wqptr = wq;
    errflag = 0;
    nxtlab = 0;

    /* M30: block-local machinery. declared MUST be -1 here. */
    declared = -1;
    ncmp = 0;
    swactive = 0;

    /* M32: switch table. swend is the CORRECTED bound -- the last
    ** valid (label,value) pair start = swstab + 178. See CC_DATA
    ** for why baseline's SWTABSZ-SWSIZ expression is a bound bug. */
    swdefault = 0;
    swnext = swstab;
    swend = swstab + 178;

    /* M33: preprocessor state. ccode YES = parsing C, not #asm. */
    ccode = 1;
    pptr = 0;
    macptr = 0;
    iflevel = 0;
    skiplevel = 0;
    incunit = 0;
    oldseg = 0;
    usexpr = 1;
    monitor = 0;
}

lout()
{
}

/*
** Get required array size. The expression evaluator lives in OVL_EXPR,
** so this resident helper crosses through the resident trampoline.
*/
needsub()
{
    int val;

    if(match("]")) return 0;

    if(R_CEXPR(&val) == 0)
        val = 1;

    if(val < 0) {
        error("negative size illegal");
        val = -val;
    }

    need("]");
    return val;
}

/*
** ==== M30: parse one declarator. MOVED here from CC_DFUN.C ====
**
** Called by doargs() (OVL_DFUN) and declloc() (OVL_STMT) -- two
** pages of the same window, so this must be resident, exactly like
** kill/illname/multidef/needsub before it. CCC1.C verbatim.
**
** aid is what an unsized "[]" decays to: doargs() passes POINTER
** (array arguments decay), declloc() passes ARRAY (locals must be
** sized). needsub() below crosses to the expression engine through
** R_CEXPR, so "int v[N];" as a local nests a trampoline frame
** inside the statement overlay's -- the deepest TRSTACK path M30
** exercises.
*/
decl(type, aid, id, sz) int type, aid, *id, *sz;
{
    int n, p;

    if(match("("))
        p = 1;
    else
        p = 0;

    if(match("*")) {
        *id = POINTER;
        *sz = BPW;
    }
    else {
        *id = VARIABLE;
        *sz = type >> 2;
    }

    if((n = symname(ssname)) == 0)
        illname();

    if(p && match(")"))
        ;

    if(match("(")) {
        if(!p || *id != POINTER)
            error("try (*...)()");
        need(")");
    }
    else if(*id == VARIABLE && match("[")) {
        *id = aid;

        if((*sz *= needsub()) == 0) {
            if(aid == ARRAY)
                error("need array size");

            *sz = BPW;          /* size of pointer argument */
        }
    }

    return n;
}

kill()
{
    *line = 0;
    bump(0);
}

illname()
{
    error("illegal symbol");
    skip();
}

multidef(sname) char *sname;
{
    error("already defined");
}

/*
** Step 6: needlval() is a CCC2 error helper, not a CCC3 one, so it
** did not come across with the expression engine -- but level1() and
** level13() call it whenever an assignment or ++/-- lands on
** something that is not an lvalue. Resident, alongside illname() and
** multidef(), because both halves of the engine reference it.
*/
needlval()
{
    error("must be lvalue");
}

/*
** Top-level parser.
** M36c1 replaces the historical #include skip with the real Hendrix
** one-level include path in OVL_PREP. M36c2 routes #asm through the
** same overlay so raw assembler blocks are copied to the output.
*/
parse()
{
    while(eof == 0) {
        if(amatch("extern", 6))
            R_DODECL(EXTERNAL);
        else if(R_DODECL(STATIC))
            ;
        else if(match("#asm"))
            doasm();
        else if(match("#include"))
            R_INCLUDE();
        else if(match("#define"))
            R_DEFINE();
        else
            R_DOFN();

        blanks();
    }
}

/*
** Existing overlay modules currently call these public names. Keep the
** names resident and forward through OVLSTUBS so no overlay calls another
** page in the same window directly.
*/
statement()
{
    return R_STMT();
}

doasm()
{
    R_DOASM();
}

/*
** Conditional-expression bridge used by CC_STMT.
**
** R_TEST preserves both arguments while mapping CC_EXPR, then restores
** whichever window-A statement page was active.
*/
test(label, parens) int label, parens;
{
    return R_TEST(label, parens);
}

constexpr(val) int *val;
{
    return R_CEXPR(val);
}

/*
** Real expression bridge used by CC_STMT.
**
** R_EXPR remains the no-argument walking-skeleton entry. R_EXPRVAL is the
** argument-preserving compiler entry.
*/
expression(con, val) int *con, *val;
{
    return R_EXPRVAL(con, val);
}

/*
** Resident literal service.
**
** CC_EXPR owns the grammar decision that a quoted argument is present,
** but byte storage belongs with resident data so it does not consume the
** scarce >8000 expression window.
*/
litput(value) int value;
{
    if(litptr >= LITMAX) {
        error("literal queue overflow");
        return 0;
    }

    litq[litptr++] = value;
    return 1;
}

/*
** Decode one source character, including the baseline common escapes.
** This depends only on resident scanner state and is therefore resident.
*/
litchar()
{
    int i;
    int oct;

    if(ch != '\\' || nch == 0)
        return gch();

    gch();

    if(ch == 'n') {
        gch();
        return 10;
    }

    if(ch == 't') {
        gch();
        return 9;
    }

    if(ch == 'b') {
        gch();
        return 8;
    }

    if(ch == 'f') {
        gch();
        return 12;
    }

    i = 3;
    oct = 0;

    while((i--) > 0 && ch >= '0' && ch <= '7')
        oct = (oct << 3) + gch() - '0';

    if(i == 2)
        return gch();

    return oct;
}

/*
** Resident integer-token service.
**
** Numeric token decoding depends only on scanner state. Keeping it resident
** leaves the expression page available for the growing run-time engine.
*/
hexval(c) int c;
{
    if(c >= '0' && c <= '9')
        return c - '0';

    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    return -1;
}

number(value) int *value;
{
    int k;
    int minus;
    int digit;

    k = 0;
    minus = 0;

    while(1) {
        if(match("+"))
            ;
        else if(match("-"))
            minus = 1;
        else
            break;
    }

    if(isdigit(ch) == 0)
        return 0;

    if(ch == '0') {
        gch();

        if(ch == 'x' || ch == 'X') {
            gch();

            while((digit = hexval(ch)) >= 0) {
                k = k * 16 + digit;
                gch();
            }
        }
        else {
            while(ch >= '0' && ch <= '7')
                k = k * 8 + gch() - '0';
        }
    }
    else {
        while(isdigit(ch))
            k = k * 10 + gch() - '0';
    }

    if(minus) {
        *value = -k;
        return INT;
    }

    *value = k;

    if(k < 0)
        return UINT;

    return INT;
}

/*
** Parse a string literal into the resident literal queue.
**
** This is scanner/literal work rather than expression precedence, so it is
** resident and leaves CC_EXPR space for the growing run-time engine.
**
** ==== STEP 6: this IS baseline string(), under its baseline name ====
**
** It was written as quoted() alongside a stub string() that returned 0,
** and nothing ever called it -- so every string literal fell through to
** experr() and the parser then walked the literal's own bytes as source.
** In milestone 28 that turned printf("%d",x) into three syntax errors, a
** phantom AUTOEXT symbol 'd', and a one-argument call.
**
** The two live call sites are constant() below (gen POINT1l) and
** CC_DECL's init(), which needs litptr to ADVANCE:
**
**     *dim -= litptr - value;
**
** so this continues to use litput(), which really stores into litq.
** M36b4 puts global numeric initializer storage in CC_DECL and retires
** the old resident stowlit spy; this remains the shared scanner path.
**
** VERIFY against src/reference/CCC3.C string() before blessing the log.
** Known divergence: on an unterminated literal this errors and returns 0,
** where baseline breaks out and returns 1. Malformed input only.
*/
string(offset) int *offset;
{
    if(match("\"") == 0)
        return 0;

    *offset = litptr;

    while(ch != '"') {
        if(ch == 0) {
            error("missing quote");
            return 0;
        }

        litput(litchar());
    }

    gch();
    litput(0);

    return 1;
}

/*
** Resident literal/constant services for the expression engine.
**
** constant() and chrcon() are CCC3 functions, but they belong with
** number(), string(), litchar() and litput() -- all scanner and
** literal work, all already resident -- rather than in the expression
** overlay. Keeping them here also buys ~200 bytes of headroom in both
** halves of the 8K pair, which the engine needs more than resident
** does (resident is at 76%, the pair is the tight resource).
**
** constant() calls gen(), which lives in CC_CODEGEN. That page is
** window B and is permanently mapped, so a direct call from resident
** code is legal.
**
** Both are Hendrix CCC3 verbatim.
*/
constant(is)  int is[]; {
  int offset;
  if     (is[TC] = number(is + CV)) gen(GETw1n,  is[CV]);
  else if(is[TC] = chrcon(is + CV)) gen(GETw1n,  is[CV]);
  else if(string(&offset))          gen(POINT1l, offset);
  else return 0;
  return 1;
  }


chrcon(value)  int *value; {
  int k;
  k = 0;
  if(match("'") == 0) return 0;
  while(ch != '\'') k = (k << 8) + (litchar() & 255);
  gch();
  *value = k;
  return (INT);
  }


/*
** Resident while-context management.
**
** Each entry contains:
**
**     WQSP    compiler stack depth at loop entry
**     WQLOOP  continue/iteration label
**     WQEXIT  break/exit label
**
** The queue is resident because compiling a condition or expression maps
** CC_EXPR over the CC_STMT window. Overlay-local control state would not
** survive that replacement reliably.
*/
addwhile(ptr) int ptr[];
{
    int k;

    if(wqptr == WQMAX) {
        error("control statement nesting limit");
        return 0;
    }

    ptr[WQSP] = csp;
    ptr[WQLOOP] = getlabel();
    ptr[WQEXIT] = getlabel();

    k = 0;
    while(k < WQSIZ)
        *wqptr++ = ptr[k++];

    return 1;
}

readwhile(ptr) int *ptr;
{
    if(ptr <= wq) {
        error("out of context");
        return 0;
    }

    return ptr - WQSIZ;
}

delwhile()
{
    if(wqptr > wq)
        wqptr = wqptr - WQSIZ;
}
