/*
** CC_MAIN.C -- vertical compiler acceptance test, M37a
**
** K&R / Hendrix Small-C v2.2 source. The compiler reads TEST.C from
** the board disk and writes the generated CCC4 8086 stream to TEST.ASM
** through the same buffered CP/M I/O library proved for input at M34.
**
** M37a retains the proven disk path and now wraps the generated
** instructions in the complete Hendrix 8086 assembler module format.
**
**   - TEST.C is opened for reading.
**   - TEST.ASM is created/truncated with fopen(...,"w").
**   - every pstr/pchar/phex/pdec/pnl codegen byte is routed through
**     putc(c,outunit), not the monitor WRITE XOP.
**   - TEST.ASM is explicitly closed so its final buffer and CP/M EOF
**     marker reach disk.
**   - the file is reopened and its leading "CODE" text is checked.
**
** TEST.C defines two functions, calls them with zero and two
** arguments, shifts by a variable count, and exercises both DBL1 and DBL2
** through commuted int-pointer addition plus pointer subtraction.
*/

extern ccinit();
extern setcodes();
extern parse();
extern header();
extern trailer();
extern R_PREP();
extern fopen();
extern fclose();
extern getc();
extern puts();

extern int srcunit;
extern int outunit;
extern int eof;

main(argc, argv)
int argc;
char **argv;
{
    int verify;
    int bad;

    puts("[CCPORT] milestone 37a: complete 8086 module to TEST.ASM\n");

    ccinit();
    setcodes();

    srcunit = fopen("TEST.C", "r");

    if(srcunit == 0) {
        puts("[CCPORT] FAIL could not open TEST.C\n");
        return;
    }

    outunit = fopen("TEST.ASM", "w");

    if(outunit == 0) {
        fclose(srcunit);
        srcunit = 0;
        puts("[CCPORT] FAIL could not create TEST.ASM\n");
        return;
    }

    eof = 0;
    R_PREP();
    header();
    parse();
    trailer();

    if(fclose(outunit) == 0) {
        outunit = 0;
        fclose(srcunit);
        srcunit = 0;
        puts("[CCPORT] FAIL could not close TEST.ASM\n");
        return;
    }
    outunit = 0;

    fclose(srcunit);
    srcunit = 0;

    if(eof == 0) {
        puts("[CCPORT] WARN parse returned before EOF\n");
        return;
    }

    verify = fopen("TEST.ASM", "r");
    if(verify == 0) {
        puts("[CCPORT] FAIL could not reopen TEST.ASM\n");
        return;
    }

    bad = 0;
    if(getc(verify) != 'C') bad = 1;
    if(getc(verify) != 'O') bad = 1;
    if(getc(verify) != 'D') bad = 1;
    if(getc(verify) != 'E') bad = 1;
    fclose(verify);

    if(bad) {
        puts("[CCPORT] FAIL TEST.ASM readback did not start CODE\n");
        return;
    }

    puts("[CCPORT] DONE M37a wrote and reopened 8086 TEST.ASM\n");
}
