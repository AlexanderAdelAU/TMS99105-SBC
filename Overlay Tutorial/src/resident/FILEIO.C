/*
** FILEIO.C -- FILESTAT resident file services.
**
** The open file and filename remain resident so every analysis overlay can
** use the same file without owning persistent state.
*/

extern fopen();
extern fclose();
extern fgetc();
extern putchar();
extern puts();

extern char *fsname;
extern int fsunit;

fsopen()
{
    fsunit = fopen(fsname, "r");
    if(fsunit == 0) {
        puts("FILESTAT: cannot open ");
        puts(fsname);
        puts("\n");
        return 0;
    }
    return 1;
}

fsclose()
{
    if(fsunit) {
        fclose(fsunit);
        fsunit = 0;
    }
}

fsrewind()
{
    fsclose();
    return fsopen();
}

fsgetc()
{
    return fgetc(fsunit);
}

/*
** On this SBC, IOLIB returns '\n' as CR (0x0D) for a logical line ending.
** Console character output therefore adds LF (0x0A) after CR.
*/
fsout(c)
int c;
{
    putchar(c);
    if(c == '\n')
        putchar(10);
}

fslist()
{
    int c;

    puts("\n--- file contents ---\n");
    c = fsgetc();
    while(c != -1) {
        fsout(c);
        c = fsgetc();
    }
    puts("--- end contents ---\n");
}
