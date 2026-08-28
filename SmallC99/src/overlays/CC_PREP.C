#asm
	AORG 8000H
#endasm

/*
** CC_PREP.C -- CCC2 preprocessor overlay (OVL_PREP), through M36c2
**
** DREL ANCHORS: preprocess(), dodefine(), doinclude() and doasm().
** All are resident-callable entry points (P_PREP, P_DEFINE,
** P_INCLUDE and P_DOASM in OVLADDR.INC); any one tags
** this module as OVL_PREP; DREL 3.8+ makes an untagged paged module a
** hard error rather than a silently unmapped page.
**
** WHY THE PREPROCESSOR IS AN OVERLAY
**
** Resident is the only scarce memory on this machine: >1000-7FFF is
** 28672 bytes and milestone 32 left 970 free, while the board has 15
** pages available per segment. The rule that falls out -- and this is
** its first application -- is that anything touched in only ONE PHASE
** of compilation belongs in a page, not resident.
**
** The preprocessor qualifies completely. preprocess() is reached from
** exactly one place, blanks(), and only when the line buffer empties:
** ONCE PER SOURCE LINE, not per token. Everything it touches -- macn,
** macq, keepch, ifline, inline -- is private to it. So code AND macro
** pools live here, and resident pays only for the two line buffers the
** scanner must see from every page (pline, mline in CC_SCAN_SYM).
**
** Entry is through R_PREP / R_DEFINE / R_INCLUDE in OVLSTUBS, the
** same TRSTACK
** trampoline pattern as R_STMT: blanks() can be called from inside the
** expression engine, so the caller's window-A page must be saved and
** restored around the remap.
**
** DO NOT write "extern name();" for a resident function used here.
** smallcp emits an ENT for that, this module exports its own stub, and
** the call binds to the stub instead of the real function -- silent at
** link time, a wild call at run time. Undeclared calls emit a clean
** EXT, which is what is wanted.
**
** ==== TWO PAGES, ONE OVERLAY ID ====
**
** OVL_PREP owns TWO 4KB pages, mapped simultaneously exactly as the
** expression engine's four are, so references between them are
** ordinary and direct:
**
**     CC_PREP.C   AORG >8000  segment 8   code
**     CC_MACS.C   AORG >9000  segment 9   macn/macq pools + putmac()
**
** The split is forced by measurement, not taste: this code assembles
** to about 2418 bytes, and pools big enough for self-hosting are
** another 3600. Together they overran one page by 482 bytes at
** MACNBR 120 (DREL spec 2.5). Splitting gives the code 59% of its own
** page and hands the pools a whole page, which is what lets MACNBR be
** 200 instead of a self-hosting time bomb.
**
** DREL tags CC_MACS through putmac(); every module of a multi-page
** overlay must carry a recognised anchor or it gets no table row, is
** never mapped, and the first reference into it lands in unmapped
** memory.
**
** ==== M34 SEAM ====
**
** inline() below is the ONLY function that knows where source text
** comes from. It reads from a memory buffer today; at M34 it becomes
** the real file reader (the IOCORE/IOREAD modules have been linked and
** waiting since the beginning) and NOTHING ELSE IN THE COMPILER
** CHANGES. That is why the seam is cut here and not inside
** preprocess().
**
** ==== SCOPE ====
**
** ifline() is included VERBATIM, so #ifdef/#ifndef/#else/#endif are
** live code as of this milestone even though the M33a fixture does not
** exercise them -- policy is reference-faithful, and preprocess()
** calls ifline() unconditionally. M33b tests them; no new code will be
** needed for that, only a fixture.
**
** M36c1 ports Hendrix's one-level #include path.
** M36c2 ports Hendrix doasm(): raw source lines are copied directly
** to the generated assembly stream until #endasm, without macro
** expansion or comment stripping. The same routine is used at file
** scope, statement scope, and while an include file is active.
*/

/*
** ==== LINE TERMINATORS ====
**
** THIS smallcp COMPILES '\n' AS 0x0D (CR), NOT 0x0A.
**
** Proven from the DREL dump of CC_MAIN.R99: the fixture line
** "#define MAX 3\n" assembles to
**     23 64 65 66 69 6E 65 20 4D 41 58 20 33 0D 00
** ending CR, NUL. Every puts("...\n") in the harness does the same,
** which is why terminal output has looked correct all along.
**
** M33a shipped with NEWLINE 10 and cost a debugging session: inline()
** never found a line end, swallowed the whole 101-byte fixture as one
** line, and dodefine()'s "while(putmac(gch()))" -- which consumes to
** end of line -- ate the entire rest of the program as MAX's
** replacement text. parse() then saw eof with nothing compiled, so the
** p-code log came out EMPTY.
**
** CR and LF are both recognised as terminators by inline() below and
** normalised to one NEWLINE, so this survives M34 when real CP/M files
** (CR+LF) replace the memory source.
*/
#define NEWLINE  13
#define CR       13
#define LF       10
#define CTRLZ    26
#define NULL      0
#define YES       1
#define NO        0

#define NAMESIZE  9
#define NAMEMAX   8
#define LINEMAX 127

/*
** Macro pool sizing. MUST MATCH CC_MACS.C EXACTLY -- the pools are
** declared there and these defines describe them from here.
**
** With a page to themselves the pools are no longer the constraint:
** 200 macros cost 3600 of 4096 bytes. Baseline MACNBR is 300; 200 is
** chosen to leave putmac() and headroom in that page, and the largest
** source in this project uses about 85 #defines, so self-hosting
** clears with real margin now rather than by a hair.
**
** search() wraps forever on a full table rather than reporting it, so
** watch the page budget line if this is ever raised.
*/
#define MACNBR   200
#define MACNSIZE (MACNBR*(NAMESIZE+2))
#define MACQSIZE (MACNBR*7)
#define MACMAX   (MACQSIZE-1)
#define MACNEND  (macn+MACNSIZE)

extern char *line;
extern char *lptr;
extern char *cptr;
extern char *cptr2;
extern int srcunit;    /* open primary source-file unit */
extern int incunit;    /* open one-level include-file unit, or zero */

extern char mline[];
extern char pline[];
extern char msname[];

extern int ch;
extern int nch;
extern int eof;
extern int errflag;
extern int ccode;
extern int pptr;
extern int macptr;
extern int iflevel;
extern int skiplevel;

extern int getc();

/* Defined in CC_MACS.C -- the second page of this same overlay. */
extern char macn[];
extern char macq[];

/*
** Fetch one source line into the buffer "line" points at.
**
** ==== THIS IS THE M34 FILE-INPUT SEAM ====
**
** Memory version: srcptr walked a NEWLINE-separated source image.
** M34 replaced it with file input; M36c1 adds Hendrix's one-secondary-
** input include stream here and nowhere else in the scanner.
**
** Contract, which the file version must honour exactly:
**   - copy one line INCLUDING its NEWLINE into *line
**   - NUL-terminate
**   - set eof = 1 and store an empty line when the source is exhausted
**   - end with bump(0) so ch/nch/lptr describe the new line
*/
inline()
{
    int k;
    int c;
    int unit;

    /*
    ** M36c1: preserve the Hendrix 2.2 input/input2 model.
    **
    ** incunit, when nonzero, temporarily supplies lines. At its EOF it
    ** is closed and reading resumes from srcunit in the SAME inline()
    ** call. Only exhaustion of srcunit sets the compiler-wide eof flag.
    ** This is deliberately one-level include support: baseline has one
    ** input2 slot, not a recursive include stack.
    */
    while(1) {
        if(incunit)
            unit = incunit;
        else
            unit = srcunit;

        /* A compiler never reads the console as a source stream. */
        if(unit == 0) {
            eof = YES;
            line[0] = NULL;
            bump(0);
            return;
        }

        c = getc(unit);

        if(c == -1 || c == CTRLZ) {
            if(incunit) {
                fclose(incunit);
                incunit = 0;
                continue;
            }

            eof = YES;
            line[0] = NULL;
            bump(0);
            return;
        }

        k = 0;
        while(k < LINEMAX) {
            if(c == CR) {
                line[k++] = NEWLINE;
                break;
            }

            line[k++] = c;
            c = getc(unit);

            if(c == -1 || c == CTRLZ) {
                /* Complete an unterminated final line. If it belonged
                ** to an include, close that include but do not mark the
                ** primary source EOF. */
                line[k++] = NEWLINE;
                if(incunit) {
                    fclose(incunit);
                    incunit = 0;
                }
                else
                    eof = YES;
                break;
            }
        }

        line[k] = NULL;
        bump(0);
        return;
    }
}

/*
** Deposit one character in the processed line. CCC2 keepch().
*/
keepch(c) char c;
{
    if(pptr < LINEMAX)
        pline[++pptr] = c;
}

/*
** Conditional-compilation error. CCC2 noiferr().
*/
noiferr()
{
    error("no matching #if...");
    errflag = 0;
}

/*
** Fetch the next line that is not a conditional directive and not
** inside a skipped conditional. CCC2 ifline(), verbatim.
*/
ifline()
{
    while(1) {
        inline();
        if(eof) return;

        if(match("#ifdef")) {
            ++iflevel;
            if(skiplevel) continue;
            symname(msname);
            if(search(msname, macn, NAMESIZE+2, MACNEND, MACNBR, 0) == 0)
                skiplevel = iflevel;
            continue;
        }

        if(match("#ifndef")) {
            ++iflevel;
            if(skiplevel) continue;
            symname(msname);
            if(search(msname, macn, NAMESIZE+2, MACNEND, MACNBR, 0))
                skiplevel = iflevel;
            continue;
        }

        if(match("#else")) {
            if(iflevel) {
                if(skiplevel == iflevel) skiplevel = 0;
                else if(skiplevel == 0)  skiplevel = iflevel;
            }
            else noiferr();
            continue;
        }

        if(match("#endif")) {
            if(iflevel) {
                if(skiplevel == iflevel) skiplevel = 0;
                --iflevel;
            }
            else noiferr();
            continue;
        }

        if(skiplevel) continue;
        if(ch == 0) continue;
        break;
    }
}

/*
** Read and preprocess one source line. CCC2 preprocess(), verbatim.
**
** Strips comments, collapses white space, passes string and character
** literals through untouched, and expands macros. The scanner then
** walks pline and never sees macn/macq at all -- which is exactly why
** the pools can live out here in a page.
**
** Reached ONLY through R_PREP (from resident blanks()).
*/
preprocess()
{
    int k;
    char c;

    if(ccode) {
        line = mline;
        ifline();
        if(eof) return;
    }
    else {
        inline();
        return;
    }

    pptr = -1;

    while(ch != NEWLINE && ch) {
        if(white()) {
            keepch(' ');
            while(white()) gch();
        }
        else if(ch == '"') {
            keepch(ch);
            gch();
            while(ch != '"' || (*(lptr-1) == 92 && *(lptr-2) != 92)) {
                if(ch == NULL) {
                    error("no quote");
                    break;
                }
                keepch(gch());
            }
            gch();
            keepch('"');
        }
        else if(ch == 39) {
            keepch(39);
            gch();
            while(ch != 39 || (*(lptr-1) == 92 && *(lptr-2) != 92)) {
                if(ch == NULL) {
                    error("no apostrophe");
                    break;
                }
                keepch(gch());
            }
            gch();
            keepch(39);
        }
        else if(ch == '/' && nch == '*') {
            bump(2);
            while((ch == '*' && nch == '/') == 0) {
                if(ch) bump(1);
                else {
                    ifline();
                    if(eof) break;
                }
            }
            bump(2);
        }
        else if(an(ch)) {
            k = 0;
            while(an(ch) && k < NAMEMAX) {
                msname[k++] = ch;
                gch();
            }
            msname[k] = NULL;
            if(search(msname, macn, NAMESIZE+2, MACNEND, MACNBR, 0)) {
                k = getint(cptr+NAMESIZE, 2);
                while(c = macq[k++]) keepch(c);
                while(an(ch)) gch();
            }
            else {
                k = 0;
                while(c = msname[k++]) keepch(c);
            }
        }
        else keepch(gch());
    }

    if(pptr >= LINEMAX) error("line too long");
    keepch(NULL);
    line = pline;
    bump(0);
}

/*
** Define one macro. CCC1 dodefine(), verbatim.
**
** Reached ONLY through R_DEFINE (from resident parse()), which replaced
** the milestone-28 "#define -> skip()" stub.
**
** Baseline calls abort(ERRCODE) on a full macro string queue; this port
** has no abort, and the established milestone-30 adaptation is to
** error() and return -- the same treatment addwhile() got.
*/
dodefine()
{
    int k;

    if(symname(msname) == 0) {
        illname();
        kill();
        return;
    }

    k = 0;
    if(search(msname, macn, NAMESIZE+2, MACNEND, MACNBR, 0) == 0) {
        if(cptr2 = cptr)
            while(*cptr2++ = msname[k++]) ;
        else {
            error("macro name table full");
            return;
        }
    }

    putint(macptr, cptr+NAMESIZE, 2);
    while(white()) gch();
    while(putmac(gch())) ;

    if(macptr >= MACMAX)
        error("macro string queue full");
}

/*
** Open one include file. CCC1 doinclude(), adapted only for this port's
** zero-valued file-unit convention and explicit one-level contract.
**
** Called from resident parse() through R_INCLUDE/P_INCLUDE. The current
** preprocessed directive line remains active while the filename is
** copied. kill() then forces the next blanks()/R_PREP to fetch from the
** newly opened include unit.
*/
doinclude()
{
    int i;
    char str[30];

    blanks();
    if(*lptr == '"' || *lptr == '<') ++lptr;

    i = 0;
    while(lptr[i]
       && lptr[i] != '"'
       && lptr[i] != '>'
       && lptr[i] != NEWLINE) {
        str[i] = lptr[i];
        ++i;
    }
    str[i] = NULL;

    if(i == 0) {
        error("missing include file");
        kill();
        return;
    }

    if(incunit) {
        error("nested include not supported");
        kill();
        return;
    }

    incunit = fopen(str, "r");
    if(incunit == 0)
        error("open failure on include file");

    kill();
}
/*
** Copy one raw assembler block to the generated output. CCC1 doasm(),
** adapted only from fputs(line,output) to this port's resident pstr()
** output path.
**
** ccode == NO makes preprocess() fetch raw lines through inline().
** Because inline() already selects incunit before srcunit, #asm works
** identically in the primary source and in the active include file.
** The #asm and #endasm directive lines themselves are not emitted.
*/
doasm()
{
    ccode = NO;

    while(1) {
        inline();
        if(match("#endasm"))
            break;
        if(eof)
            break;
        pstr(line);
    }

    kill();
    ccode = YES;
}
