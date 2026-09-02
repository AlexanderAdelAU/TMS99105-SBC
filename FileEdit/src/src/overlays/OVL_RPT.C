/* OVL_RPT.C -- FILESTAT formatted-report overlay. */
#asm
        AORG    08000H
#endasm

extern puts();
extern putnum();
extern putstat();

extern int nline;
extern int nword;
extern int nchar;
extern int nalpha;
extern int ndigit;
extern int nspace;
extern int npunct;
extern int longlen;
extern char longword[];
extern char *fsname;

report()
{
    puts("\nFILESTAT report\n");
    puts("File: ");
    puts(fsname);
    puts("\n");

    putstat("Lines", nline);
    putstat("Words", nword);
    putstat("Characters", nchar);
    putstat("Letters", nalpha);
    putstat("Digits", ndigit);
    putstat("Whitespace", nspace);
    putstat("Punctuation", npunct);

    puts("Longest word: ");
    if(longword[0])
        puts(longword);
    else
        puts("(none)");
    puts(" (");
    putnum(longlen);
    puts(")\n");
}
