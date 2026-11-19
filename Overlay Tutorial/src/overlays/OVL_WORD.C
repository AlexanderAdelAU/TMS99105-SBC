/*
** OVL_WORD.C -- FILESTAT word and line analysis overlay.
**
** This module runs at >8000.  Results needed after it returns are written to
** resident FILEDATA storage.
*/
#asm
        AORG    08000H
#endasm

extern fsgetc();
extern noteword();
extern int nline;
extern int nword;

isword(c)
int c;
{
    if(c >= 'A' && c <= 'Z')
        return 1;
    if(c >= 'a' && c <= 'z')
        return 1;
    if(c >= '0' && c <= '9')
        return 1;
    if(c == '_')
        return 1;
    return 0;
}

wordscan()
{
    char word[32];
    int c;
    int inword;
    int len;
    int saw;
    int lastnl;

    inword = 0;
    len = 0;
    saw = 0;
    lastnl = 1;

    c = fsgetc();
    while(c != -1) {
        saw = 1;

        if(c == '\n') {
            nline = nline + 1;
            lastnl = 1;
        }
        else
            lastnl = 0;

        if(isword(c)) {
            if(inword == 0) {
                inword = 1;
                len = 0;
                nword = nword + 1;
            }
            if(len < 31)
                word[len] = c;
            len = len + 1;
        }
        else if(inword) {
            noteword(word, len);
            inword = 0;
            len = 0;
        }

        c = fsgetc();
    }

    if(inword)
        noteword(word, len);

    /* Count a final line that has no terminating newline. */
    if(saw && lastnl == 0)
        nline = nline + 1;
}
