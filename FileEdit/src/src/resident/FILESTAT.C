/*
** FILESTAT.C -- FileEdit resident program driver (Step 7.1 INT4 RX queue test).
**
** The original analyser and /L path are preserved.  /V is the read-only
** full-screen viewer.  /E now starts a persistent editor shell with Open,
** New, Save and Save As; it may be entered with or without an initial file.
*/

extern puts();
extern ovlinit();
extern fsset();
extern fsreset();
extern fsopen();
extern fsclose();
extern fsrewind();
extern fslist();
extern runword();
extern runchar();
extern runrpt();
extern viewctl();
extern editmenu();
extern ebload();
extern rxinit();
extern rxfini();
extern rxtest();
extern putnum();
extern int rxticks;

extern char *fsname;
extern int dolst;

islist(p)
char *p;
{
    if(p[0] != '/' && p[0] != '-')
        return 0;
    if(p[1] != 'L' && p[1] != 'l')
        return 0;
    if(p[2] != 0)
        return 0;
    return 1;
}

isview(p)
char *p;
{
    if(p[0] != '/' && p[0] != '-')
        return 0;
    if(p[1] != 'V' && p[1] != 'v')
        return 0;
    if(p[2] != 0)
        return 0;
    return 1;
}

isedit(p)
char *p;
{
    if(p[0] != '/' && p[0] != '-')
        return 0;
    if(p[1] != 'E' && p[1] != 'e')
        return 0;
    if(p[2] != 0)
        return 0;
    return 1;
}

isint(p)
char *p;
{
    if(p[0] != '/' && p[0] != '-')
        return 0;
    if(p[1] != 'I' && p[1] != 'i')
        return 0;
    if(p[2] != 0)
        return 0;
    return 1;
}

usage()
{
    puts("FILEEDIT - overlay text editor with FILESTAT analysis\n");
    puts("Usage: FILEEDIT filename [/L | /V | /E]\n");
    puts("       FILEEDIT /E\n");
    puts("       FILEEDIT /I   INT4 timer-hook diagnostic\n");
    puts("  /L or -L lists the file before analysis.\n");
    puts("  /V or -V opens the file in the Tera Term viewer.\n");
    puts("  /E or -E starts the interactive editor; filename is optional.\n");
}

main(argc, argv)
int argc;
char **argv;
{
    int doview;
    int doedit;

    ovlinit();
    fsset("");

    /* Step 7.1.3: isolated known-good TMS9902 timer/INT4 hook test. */
    if(argc == 2 && isint(argv[1])) {
        puts("FILEEDIT INT4 timer-hook test...\n");
        puts("Waiting for 8 TMS9902 timer interrupts.\n");
        rxtest();
        puts("PASS - INT4 hook received ");
        putnum(rxticks);
        puts(" timer interrupts.\n");
        return;
    }

    /* Step 6 may start directly at the editor with no initial document. */
    if(argc == 2 && isedit(argv[1])) {
        rxinit();
        editmenu();
        rxfini();
        return;
    }

    if(argc < 2) {
        usage();
        return;
    }

    fsset(argv[1]);
    dolst = 0;
    doview = 0;
    doedit = 0;

    if(argc > 2) {
        if(islist(argv[2]))
            dolst = 1;
        else if(isview(argv[2]))
            doview = 1;
        else if(isedit(argv[2]))
            doedit = 1;
        else {
            usage();
            return;
        }
    }

    if(argc > 3) {
        usage();
        return;
    }

    if(doedit) {
        rxinit();
        editmenu();
        rxfini();
        return;
    }

    fsreset();

    if(doview) {
        if(ebload() == 0)
            return;
        rxinit();
        viewctl();
        rxfini();
        return;
    }

    if(fsopen() == 0)
        return;

    if(dolst) {
        fslist();
        if(fsrewind() == 0)
            return;
    }

    runword();

    if(fsrewind() == 0)
        return;

    runchar();
    fsclose();
    runrpt();
}
