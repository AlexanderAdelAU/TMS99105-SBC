#asm
	AORG 8000H
#endasm

/*
** CC_STMT.C -- statement overlay, milestone 30
**
** M30 brings the module back to CCC1.C letter-for-letter wherever a
** reference function exists. New this milestone:
**
**     block-local declarations   statement()/declloc()/compound()
**     lazy local allocation      one ADDSP per block, emitted by
**                                statement() on the first
**                                non-declaration statement
**     label retention            compound() compacts the local
**                                table, keeping LABEL entries
**
** Also reverted to reference in this pass (they had drifted):
**
**     doif    else-arm JMPm is now guarded by
**             lastst != STRETURN && lastst != STGOTO
**     dofor   addwhile() before "(", labels allocated after it --
**             label NUMBERING differs from M29-era code (for was
**             never in a blessed log, so no re-bless)
**     dodo    WQLOOP back at the TOP of the body and ns() last:
**             reference "continue" in do/while re-enters the body
**             WITHOUT re-testing. The M29 bodylab variant was
**             arguably better C; it was not CCC1.
**     statement  goto/label dispatch precedes return/break/
**             continue, per reference; stgoto() wrapper retired
**
** statement() and compound() take NO arguments in the reference:
** the function-body vs nested-block distinction is ncmp, not a
** parameter. R_STMT, OVLSTUBS and the resident statement() shim
** are unchanged.
**
** declloc() calls decl(), which as of M30 is RESIDENT
** (CC_RESIDENT.C) -- it was in CC_DFUN.C, a different page of this
** same window, and a direct call there would have been silently
** resolved by the linker into this module's own bytes. Do NOT
** write "extern decl();" -- see the CC_EXPR header for why that
** binds a local stub; the undeclared call emits a clean EXT.
**
** M32: doswitch/docase/dodefault are now real (CCC1).
** M36c2 routes statement-scope #asm through the resident doasm()
** shim, which safely maps OVL_PREP, copies raw lines, and restores
** this page.
*/

#define STIF      1
#define STWHILE   2
#define STRETURN  3
#define STBREAK   4
#define STCONT    5
#define STASM     6
#define STEXPR    7
#define STDO      8
#define STFOR     9
#define STSWITCH 10
#define STCASE   11
#define STDEF    12
#define STGOTO   13
#define STLABEL  14

#define NO        0
#define YES       1

#define IDENT     0
#define TYPE      1
#define CLASS     2
#define OFFSET    5
#define NAME      7

#define LABEL     0
#define ARRAY     2

#define CHR       4
#define INT       8
#define UCHR      5
#define UINT      9

#define AUTOMATIC 1

#define NAMESIZE  9

#define ADDSP     2
#define WORDn    38
#define JMPm     40
#define LABm     41
#define NEARm    56
#define RETURN   67
#define SWITCH   72

#define WQSIZ   3
#define WQSP    0
#define WQLOOP  1
#define WQEXIT  2

extern int ch;
extern int eof;
extern int errflag;
extern int lastst;
extern int csp;
extern int usexpr;
extern int wq[];
extern int *wqptr;
extern int nogo;
extern int noloc;

extern int declared;    /* # local bytes pending, -1 when flushed */
extern int ncmp;        /* # open compound statements             */
extern int swactive;    /* inside a switch?                       */
extern int swdefault;   /* default label #, else 0                */
extern int *swnext;     /* next free (label,value) slot in swstab */
extern int *swend;      /* last valid slot start (see CC_DATA)    */

extern char *locptr;
extern char *lptr;
extern char *cptr;      /* char* -- compound() walks bytes */
extern char *cptr2;
extern char ssname[];

/*
** ==== M32b: SEVEN FUNCTIONS RELOCATED TO CC_RESIDENT.C ====
**
** doexpr, dobreak, docont, dogoto, dolabel, addlabel and dodefault
** now live resident. The M32 switch trio pushed this module to
** 0x12A0 = 4768 bytes, 672 past its 4KB page (DREL spec 2.5,
** SEGMENT OVERRUN). These seven are the page-independent leaves --
** each calls only resident services, never a same-page sibling --
** so they join kill/illname/multidef/needsub/decl below the
** window. Calls from this page resolve as clean EXTs; do NOT add
** "extern" declarations for them (see the CC_EXPR header).
**
** Pure relocation: zero p-code changes, all blessed logs valid.
*/

/*
** Parse one statement. CCC1 statement().
**
** The "declared >= 0" block is the whole local-variable stack
** mechanism: declloc() only accumulates a byte count, and the
** stack does not move until the first NON-declaration statement in
** the block arrives here. All of a block's declarations therefore
** collapse into a single ADDSP. Past that point declared is -1 and
** further declarations in the block are errors.
*/
statement()
{
    if(ch == 0 && eof)
        return lastst;

    if(amatch("char", 4)) {
        declloc(CHR);
        ns();
    }
    else if(amatch("int", 3)) {
        declloc(INT);
        ns();
    }
    else if(amatch("unsigned", 8)) {
        if(amatch("char", 4)) {
            declloc(UCHR);
            ns();
        }
        else {
            amatch("int", 3);
            declloc(UINT);
            ns();
        }
    }
    else {
        if(declared >= 0) {
            if(ncmp > 1)
                nogo = declared;        /* disable goto */
            gen(ADDSP, csp - declared);
            declared = -1;
        }

        if(match("{"))
            compound();
        else if(amatch("if", 2)) {
            doif();
            lastst = STIF;
        }
        else if(amatch("while", 5)) {
            dowhile();
            lastst = STWHILE;
        }
        else if(amatch("do", 2)) {
            dodo();
            lastst = STDO;
        }
        else if(amatch("for", 3)) {
            dofor();
            lastst = STFOR;
        }
        else if(amatch("switch", 6)) {
            doswitch();
            lastst = STSWITCH;
        }
        else if(amatch("case", 4)) {
            docase();
            lastst = STCASE;
        }
        else if(amatch("default", 7)) {
            dodefault();
            lastst = STDEF;
        }
        else if(amatch("goto", 4)) {
            dogoto();
            lastst = STGOTO;
        }
        else if(dolabel())
            lastst = STLABEL;
        else if(amatch("return", 6)) {
            doreturn();
            ns();
            lastst = STRETURN;
        }
        else if(amatch("break", 5)) {
            dobreak();
            ns();
            lastst = STBREAK;
        }
        else if(amatch("continue", 8)) {
            docont();
            ns();
            lastst = STCONT;
        }
        else if(match(";"))
            errflag = 0;
        else if(match("#asm")) {
            doasm();
            lastst = STASM;
        }
        else {
            doexpr(NO);
            ns();
            lastst = STEXPR;
        }
    }

    return lastst;
}

/*
** Declare local variables. CCC1 declloc().
**
** Never moves the stack: accumulates declared and records each
** symbol's offset as csp - declared, computed BEFORE csp moves.
** statement() emits the one ADDSP later.
**
** aid is ARRAY: an unsized "[]" is an error for a local, where
** doargs() passes POINTER because an array argument decays.
*/
declloc(type) int type;
{
    int id, sz, alloc;

    if(swactive)     error("not allowed in switch");
    if(noloc)        error("not allowed with goto");
    if(declared < 0) error("must declare first in block");

    while(1) {
        if(endst()) return;

        decl(type, ARRAY, &id, &sz);

        /* TMS ABI: sz is the C object size and must remain exact so
        ** sizeof(local) sees the logical size.  alloc is the physical
        ** stack slot, rounded to a whole word so SP and every
        ** FP-relative object address remain even.  A scalar char thus
        ** has sizeof 1 but occupies a 2-byte local stack slot; char[3]
        ** has sizeof 3 but occupies 4 bytes of stack.
        */
        alloc = sz;
        if(alloc & 1) ++alloc;

        declared += alloc;
        addsym(ssname, id, type, sz, csp - declared,
               &locptr, AUTOMATIC);

        if(match(",") == 0) return;
    }
}

/*
** CCC1 compound(). Three jobs on close:
**
**   1. gen(ADDSP, savcsp) drops this block's locals -- but only
**      when --ncmp is non-zero, i.e. NOT for the function's own
**      outermost block, whose frame RETURN takes down; and not
**      after return/goto, when the stack is already gone.
**
**   2. compact the local table, COPYING LABEL entries down over
**      dead variable entries: labels are function-scoped and must
**      survive the block that declared them. This is what needs
**      nextsym() (CC_SCAN_SYM).
**
**   3. declared = -1: the enclosing block may not declare.
*/
compound()
{
    int savcsp;
    char *savloc;

    savcsp = csp;
    savloc = locptr;
    declared = 0;               /* may now declare local variables */
    ++ncmp;                     /* new level open */

    while(match("}") == 0) {
        if(eof) {
            error("no final }");
            break;
        }
        statement();
    }

    if(--ncmp                   /* close current level */
    && lastst != STRETURN
    && lastst != STGOTO)
        gen(ADDSP, savcsp);     /* delete local variable space */

    cptr = savloc;              /* retain labels */
    while(cptr < locptr) {
        cptr2 = nextsym(cptr);
        if(cptr[IDENT] == LABEL) {
            while(cptr < cptr2)
                *savloc++ = *cptr++;
        }
        else
            cptr = cptr2;
    }

    locptr = savloc;            /* delete local symbols */
    declared = -1;              /* may not declare variables */
}


/*
** Compile return with an optional expression. CCC1 doreturn().
*/
doreturn()
{
    int savcsp;

    if(endst() == 0)
        doexpr(YES);

    savcsp = csp;
    gen(RETURN, 0);
    csp = savcsp;
}

/*
** Compile if with an optional else arm. CCC1 doif().
**
** M30: the else-arm JMPm is guarded -- after a true arm ending in
** return or goto there is nothing to jump over from.
*/
doif()
{
    int flab1, flab2;

    test(flab1 = getlabel(), YES);  /* get expr, branch false */
    statement();                    /* if true, do a statement */

    if(amatch("else", 4) == 0) {    /* if...else ? */
        gen(LABm, flab1);           /* simple if: false label */
        return;
    }

    flab2 = getlabel();

    if(lastst != STRETURN && lastst != STGOTO)
        gen(JMPm, flab2);

    gen(LABm, flab1);               /* false label */
    statement();                    /* else clause */
    gen(LABm, flab2);               /* true label */
}

/*
** Compile a while loop. CCC1 dowhile(), using the resident context
** queue: addwhile() records csp and both labels before any
** expression overlay is mapped over this page.
**
** The port's addwhile() returns 0 on queue overflow where baseline
** aborts; the guard is the established M29 adaptation.
*/
dowhile()
{
    int entry[WQSIZ];

    if(addwhile(entry) == 0)
        return;

    gen(LABm, entry[WQLOOP]);   /* loop label */
    test(entry[WQEXIT], YES);   /* see if true */
    statement();                /* if so, do a statement */
    gen(JMPm, entry[WQLOOP]);   /* loop to label */
    gen(LABm, entry[WQEXIT]);   /* exit label */

    delwhile();                 /* delete queue entry */
}

/*
** Compile a post-tested do/while loop. CCC1 dodo(), restored:
**
**     LABm WQLOOP
**     statement()
**     "while" test(WQEXIT, YES)
**     JMPm WQLOOP
**     LABm WQEXIT
**     ns()
**
** WQLOOP is the TOP of the body: reference "continue" in do/while
** re-enters the body without re-testing the condition. The M29
** bodylab variant (test at continue) was better-behaved C but was
** not CCC1, and M30 policy is reference letter-for-letter.
*/
dodo()
{
    int entry[WQSIZ];

    if(addwhile(entry) == 0)
        return;

    gen(LABm, entry[WQLOOP]);
    statement();

    need("while");
    test(entry[WQEXIT], YES);

    gen(JMPm, entry[WQLOOP]);
    gen(LABm, entry[WQEXIT]);

    delwhile();
    ns();
}

/*
** Compile a for loop. CCC1 dofor(), restored: addwhile() FIRST,
** then lab1/lab2, then "(". Label numbering therefore differs from
** the M29-era code -- harmless, "for" has never been in a blessed
** log. Shape:
**
**     expr1
**     LABm lab1
**     test(WQEXIT, NO)        (omitted condition -> fall through)
**     JMPm lab2
**     LABm WQLOOP
**     expr3
**     JMPm lab1
**     LABm lab2
**     statement()
**     JMPm WQLOOP
**     LABm WQEXIT
**
** WQLOOP remains the increment position even when expr3 is empty,
** so continue keeps the correct C meaning.
*/
dofor()
{
    int entry[WQSIZ];
    int lab1, lab2;

    if(addwhile(entry) == 0)
        return;

    lab1 = getlabel();
    lab2 = getlabel();

    need("(");

    if(match(";") == 0) {
        doexpr(NO);             /* expr 1 */
        ns();
    }

    gen(LABm, lab1);

    if(match(";") == 0) {
        test(entry[WQEXIT], NO); /* expr 2 */
        ns();
    }

    gen(JMPm, lab2);

    gen(LABm, entry[WQLOOP]);

    if(match(")") == 0) {
        doexpr(NO);             /* expr 3 */
        need(")");
    }

    gen(JMPm, lab1);

    gen(LABm, lab2);
    statement();
    gen(JMPm, entry[WQLOOP]);
    gen(LABm, entry[WQEXIT]);

    delwhile();
}






/*
** Compile a switch. CCC1 doswitch(), with the port's addwhile()
** guard (established in M30 for dowhile/dodo/dofor) and the port's
** entry[] naming for the local queue record.
**
** The entry pushed by addwhile() gets its WQLOOP zeroed: a switch
** is a break target but never a continue target, and docont()
** already skips zero-WQLOOP entries by design.
**
** Cases accumulate (label,value) pairs in resident swstab through
** swnext -- resident because constexpr() maps CC_EXPR over this
** page while parsing each case value. On close, the collected
** pairs are emitted after the body:
**
**     JMPm endlab            jump over the body
**     <body, LABm per case>
**     JMPm WQEXIT            body fall-off exits
**     LABm endlab
**     SWITCH                 runtime case matcher
**     NEARm label WORDn value    ... one pair per case
**     WORDn 0                terminator
**     JMPm swdefault         only if a default was seen
**     LABm WQEXIT
**
** Save/restore of swactive/swdefault/swnext makes nesting work.
*/
doswitch()
{
    int entry[WQSIZ];
    int endlab, swact, swdef;
    int *swnex, *swptr;

    swact = swactive;
    swdef = swdefault;
    swnex = swptr = swnext;

    if(addwhile(entry) == 0)
        return;

    *(wqptr + WQLOOP - WQSIZ) = 0;  /* not a continue target */

    need("(");
    doexpr(YES);
    need(")");

    swdefault = 0;
    swactive = 1;

    gen(JMPm, endlab = getlabel());

    statement();                    /* cases, etc. */

    gen(JMPm, entry[WQEXIT]);
    gen(LABm, endlab);
    gen(SWITCH, 0);                 /* match cases */

    while(swptr < swnext) {
        gen(NEARm, *swptr++);
        gen(WORDn, *swptr++);       /* case value */
    }

    gen(WORDn, 0);

    if(swdefault)
        gen(JMPm, swdefault);

    gen(LABm, entry[WQEXIT]);
    delwhile();

    swnext    = swnex;
    swdefault = swdef;
    swactive  = swact;
}

/*
** Compile one case label. CCC1 docase(). constexpr() crosses to the
** expression engine through R_CEXPR, so swnext MUST be resident.
** The swnext > swend bound is real here -- see the CC_DATA note on
** the baseline calloc/scaled-pointer bound bug.
*/
docase()
{
    if(swactive == 0)
        error("not in switch");

    if(swnext > swend) {
        error("too many cases");
        return;
    }

    gen(LABm, *swnext++ = getlabel());
    constexpr(swnext++);
    need(":");
}
