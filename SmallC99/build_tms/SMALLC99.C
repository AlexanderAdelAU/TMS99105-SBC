/*
** SMALLC99.C -- M38d native TMS99000/SBC compiler driver
**
** Active TMS99105 SBC driver, built as SMALLC99.EXE. The older 8086
** driver remains in SMALLC99.C for reference only. This driver reuses
** the common Hendrix front end and overlay manager with the CC_CD99
** resident output layer and CC_CG99 template overlay.
*/

#define YES 1
#define NO  0
#define NAMEMAX 14

extern PORT_INIT();
extern R_GETOPTS();
extern R_PREP();
extern ccinit();
extern setcodes();
extern parse();
extern header();
extern trailer();
extern fopen();
extern fclose();
extern puts();

extern int srcunit;
extern int outunit;
extern int eof;
extern int tmsfail;

char srcname[NAMEMAX+1];
char outname[NAMEMAX+1];
int verbose;

main(argc, argv)
int argc;
char **argv;
{
    srcunit = 0;
    outunit = 0;
    verbose = NO;

    PORT_INIT();                 /* initialise target environment */
    if(ask(argc, argv) == NO)   /* get user options */
        return;
    compinit();                 /* initialise compiler and TMS backend */
    if(openfile() == NO)        /* open initial input and output */
        return;
    firstline();                /* fetch first source line */
    header();                   /* emit native TMS module preamble */
    parse();                    /* process ALL input */
    trailer();                  /* close native TMS module */
    if(closefile() == NO)       /* explicitly close both files */
        return;
    finish();                   /* report compiler completion */
}

ask(argc, argv)
int argc;
char **argv;
{
    if(R_GETOPTS(argc, argv) == NO)
        return NO;

    if(verbose) {
        puts("SMALLC99: compiling ");
        puts(srcname);
        puts(" -> ");
        puts(outname);
        puts("\n");
    }
    return YES;
}

compinit()
{
    ccinit();
    setcodes();
}

openfile()
{
    srcunit = fopen(srcname, "r");
    if(srcunit == 0) {
        puts("SMALLC99: cannot open ");
        puts(srcname);
        puts("\n");
        return NO;
    }

    outunit = fopen(outname, "w");
    if(outunit == 0) {
        fclose(srcunit);
        srcunit = 0;
        puts("SMALLC99: cannot create ");
        puts(outname);
        puts("\n");
        return NO;
    }
    return YES;
}

firstline()
{
    eof = 0;
    R_PREP();
}

closefile()
{
    if(fclose(outunit) == 0) {
        outunit = 0;
        fclose(srcunit);
        srcunit = 0;
        puts("SMALLC99: cannot close ");
        puts(outname);
        puts("\n");
        return NO;
    }
    outunit = 0;

    fclose(srcunit);
    srcunit = 0;
    return YES;
}

finish()
{
    if(tmsfail) {
        puts("SMALLC99: compilation failed; output is not valid assembly\n");
        return;
    }

    if(eof == 0)
        puts("SMALLC99: warning: parse returned before EOF\n");

    puts("SMALLC99: wrote ");
    puts(outname);
    puts("\n");
}
