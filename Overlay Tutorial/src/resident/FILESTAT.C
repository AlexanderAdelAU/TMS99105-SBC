/*
** FILESTAT.C -- FILESTAT resident program driver.
**
** Build this module normally.  It owns the program startup path.  Resident
** helper modules and overlay modules are built with -M.
*/

extern puts();
extern ovlinit();
extern fsreset();
extern fsopen();
extern fsclose();
extern fsrewind();
extern fslist();
extern runword();
extern runchar();
extern runrpt();

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

usage()
{
    puts("FILESTAT - overlay text file analyser\n");
    puts("Usage: FILESTAT filename [/L]\n");
    puts("  /L or -L lists the file before analysis.\n");
}

main(argc, argv)
int argc;
char **argv;
{
    /* Shared overlay environment must be initialised before normal work. */
    ovlinit();

    if(argc < 2) {
        usage();
        return;
    }

    fsname = argv[1];
    dolst = 0;

    if(argc > 2) {
        if(islist(argv[2]))
            dolst = 1;
        else {
            usage();
            return;
        }
    }

    if(argc > 3) {
        usage();
        return;
    }

    fsreset();

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
