/* OVL_CHAR.C -- FILESTAT character-class analysis overlay. */
#asm
        AORG    08000H
#endasm

extern fsgetc();
extern int nchar;
extern int nalpha;
extern int ndigit;
extern int nspace;
extern int npunct;

charscan()
{
    int c;

    c = fsgetc();
    while(c != -1) {
        nchar = nchar + 1;

        if((c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z'))
            nalpha = nalpha + 1;
        else if(c >= '0' && c <= '9')
            ndigit = ndigit + 1;
        else if(c == ' ' || c == '\t' || c == '\n')
            nspace = nspace + 1;
        else
            npunct = npunct + 1;

        c = fsgetc();
    }
}
