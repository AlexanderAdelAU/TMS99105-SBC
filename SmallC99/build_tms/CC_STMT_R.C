/*
** CC_STMT_R.C -- resident statement services (milestone 32c)
**
** The seven page-independent statement leaves, relocated OUT of the
** OVL_STMT page when the M32 switch code pushed CC_STMT 672 bytes
** past its 4KB page (DREL spec 2.5, SEGMENT OVERRUN) -- and then
** OUT of CC_RESIDENT when they pushed THAT module past its own 4KB
** spec-2.5 cap (CC_RESIDENT was at 3902 bytes; every module,
** resident or paged, must fit 4KB). This module is the natural
** seam the linker's error message asks for.
**
** Each function calls only resident services -- never a same-page
** CC_STMT sibling -- which is what made them movable. Bodies are
** UNCHANGED from the blessed M30/M31 overlay originals: pure
** relocation, zero p-code differences, every golden log valid.
**
** Callers (statement, doreturn, dofor, doswitch in OVL_STMT) reach
** these as clean EXTs. Do NOT add "extern" declarations for them
** in overlay sources -- see the CC_EXPR header for why that binds
** a local stub instead.
*/

#define YES      1
#define NO       0

#define IDENT    0
#define TYPE     1
#define OFFSET   5
#define LABEL    0

#define WQSIZ    3
#define WQSP     0
#define WQLOOP   1
#define WQEXIT   2

#define ADDSP    2
#define JMPm    40
#define LABm    41

extern int ch;
extern int usexpr;
extern int nogo;
extern int noloc;
extern int swactive;
extern int *wqptr;

extern char *lptr;
extern char *cptr;
extern char *locptr;
extern char ssname[];

/*
** ==== M32d: switch machinery state DEFINED here ====
**
** Moved from CC_DATA when the 368 bytes pushed CC_DATA to 4338 --
** past its 4KB module cap (spec 2.5). The rebuilt link99 did not
** FATAL on that: it applied a bogus AORG SETLC fixup (base=1000,
** 0x100 bytes short) that mangled CC_DATA's initialized data --
** op[]/op2[]/usexpr/declared loaded as zeros, which is what broke
** the M32 first run (missing ADD12; phantom pretest ADDSP). This
** module is the conceptual home anyway: it is the statement
** services module, and this is statement state.
**
** Resident because docase()'s constexpr() maps CC_EXPR over the
** statement page between one case and the next.
**
** swstab holds (label,value) int pairs: 90 cases = 180 ints = 360
** bytes = baseline SWTABSZ (90*SWSIZ) BYTES, allocated statically
** instead of baseline's calloc(SWTABSZ, 1).
**
** BOUND FIX (same class as the slast bug noted in CC_CODEGEN):
** baseline sets swend = swnext+(SWTABSZ-SWSIZ), which in scaled
** int-pointer arithmetic lands 712 bytes into a 360-byte
** allocation, so its "too many cases" check can never fire before
** memory past the table is destroyed. ccinit() (CC_RESIDENT)
** computes the bound correctly: swend = swstab + 178, the last
** valid pair start. Do NOT "restore" the baseline expression.
*/
int swdefault;
int *swnext;
int *swend;
int swstab[180];

/*
** Compile one comma-separated expression list. CCC1 doexpr(),
** including the use/usexpr contract. usexpr stays resident data;
** expression() below is the resident bridge shim (R_EXPRVAL).
*/
doexpr(use) int use;
{
    int const;
    int val;
    int *before;
    int *start;

    usexpr = use;

    while(1) {
        setstage(&before, &start);
        expression(&const, &val);
        clearstage(before, start);

        if(ch != ',')
            break;

        bump(1);
    }

    usexpr = YES;
}

/*
** break restores the compiler stack to loop-entry depth, then
** jumps to the innermost active exit label. CCC1 dobreak().
*/
dobreak()
{
    int *ptr;

    ptr = readwhile(wqptr);
    if(ptr == 0)
        return;

    gen(ADDSP, ptr[WQSP]);
    gen(JMPm, ptr[WQEXIT]);
}

/*
** continue searches backward for the nearest context with a
** nonzero loop label, compatible with switch entries whose WQLOOP
** is zero. CCC1 docont().
*/
docont()
{
    int *ptr;

    ptr = wqptr;

    while(1) {
        ptr = readwhile(ptr);

        if(ptr == 0)
            return;

        if(ptr[WQLOOP])
            break;
    }

    gen(ADDSP, ptr[WQSP]);
    gen(JMPm, ptr[WQLOOP]);
}

/*
** Compile a goto. CCC1 dogoto(). The historical restriction is
** retained: once goto is used, noloc prevents later block-local
** declarations; nogo (set by statement() from declared) rejects
** goto after block-locals have made stack restoration ambiguous.
*/
dogoto()
{
    if(nogo > 0)
        error("not allowed with block-locals");
    else
        noloc = 1;

    if(symname(ssname))
        gen(JMPm, addlabel(NO));
    else
        error("bad label");

    ns();
}

/*
** Recognize and define a named label. CCC1 dolabel(). When the
** character after the identifier is not ':', bump() restores the
** scanner so the token parses normally as an expression statement.
*/
dolabel()
{
    char *savelptr;

    blanks();
    savelptr = lptr;

    if(symname(ssname)) {
        if(gch() == ':') {
            gen(LABm, addlabel(YES));
            return 1;
        }

        bump(savelptr - lptr);
    }

    return 0;
}

/*
** Find or create one function-local label symbol. CCC1 addlabel(),
** including its use of the resident cptr rather than a local.
** TYPE doubles as the defined flag for LABEL entries.
*/
addlabel(def) int def;
{
    if(cptr = findloc(ssname)) {
        if(cptr[IDENT] != LABEL)
            error("not a label");
        else if(def) {
            if(cptr[TYPE])
                error("duplicate label");
            else
                cptr[TYPE] = YES;
        }
    }
    else
        cptr = addsym(ssname, LABEL, def, 0, getlabel(),
                      &locptr, LABEL);

    return getint(cptr + OFFSET, 2);
}

/*
** Compile the default label. CCC1 dodefault().
*/
dodefault()
{
    if(swactive) {
        if(swdefault)
            error("multiple defaults");
    }
    else
        error("not in switch");

    need(":");
    gen(LABm, swdefault = getlabel());
}
