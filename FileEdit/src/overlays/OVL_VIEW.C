/*
** OVL_VIEW.C -- FileEdit Step 7 shared one-frame renderer with line gutter.
**
** OVL_VIEW remains the fourth/last code overlay in segment 8.  It renders
** either the read-only viewer (vmode=0) or the in-memory editor
** (vmode=1) from the persistent EDITBUF arena at >9000-BFFF.
**
** It owns no lifetime and reads no keyboard input.
*/
#asm
        AORG    08000H
#endasm

extern putchar();
extern puts();
extern trmesc();
extern trmrev();
extern trmnorm();
extern trmpos();
extern trmclrln();
extern trmnum();
extern trmrule();
extern char *fsname;
extern char *ebbase;
extern int eblen;
extern int eblines;
extern int vidx;
extern int vtop;
extern int vtotal;
extern int vmode;
extern int vleft;
extern int etop;
extern int eline;
extern int ecol;
extern int etotal;
extern int ebdirty;
extern edvcol();
extern int fsexist;

/* Editor gutter: five right-aligned digits plus one separator space. */
vlnum(n)
int n;
{
    if(n < 10000) putchar(' ');
    if(n < 1000) putchar(' ');
    if(n < 100) putchar(' ');
    if(n < 10) putchar(' ');
    trmnum(n);
    putchar(' ');
}

/* Render one logical buffer line on one terminal row. */
vline(row, lno)
int row, lno;
{
    int c;
    int col;
    int n;
    int skip;
    int lim;

    trmpos(row, 1);
    trmclrln();

    /* The gutter is display-only.  No line-number bytes enter EDITBUF. */
    if(vmode) {
        if(lno <= etotal)
            vlnum(lno);
        else
            puts("      ");
        lim = 73;             /* columns 7..79 belong to document text */
    }
    else
        lim = 79;

    if(vidx >= eblen) {
        /* The editor exposes one final empty logical line after a trailing CR. */
        if(vmode && lno <= etotal)
            return 1;
        return 0;
    }

    /* Horizontal offset is reserved for the editor's later long-line step. */
    skip = 0;
    while(skip < vleft && vidx < eblen && ebbase[vidx] != 13) {
        vidx = vidx + 1;
        skip = skip + 1;
    }

    if(vidx >= eblen)
        return 1;

    c = ebbase[vidx];
    vidx = vidx + 1;
    col = 1;

    while(c != 13) {
        if(c == 9) {
            n = 4 - ((col - 1) % 4);
            while(n > 0) {
                if(col <= lim)
                    putchar(' ');
                col = col + 1;
                n = n - 1;
            }
        }
        else if(c == 10) {
            /* Ignore a bare LF if one reaches the normalized buffer. */
        }
        else if(c >= 32 && c < 127) {
            if(col <= lim)
                putchar(c);
            col = col + 1;
        }
        else {
            if(col <= lim)
                putchar('.');
            col = col + 1;
        }

        if(vidx >= eblen)
            return 1;

        c = ebbase[vidx];
        vidx = vidx + 1;
    }

    return 1;
}

viewfile()
{
    int row;
    int shown;
    int last;
    int top;
    int total;

    if(vmode) {
        top = etop;
        total = etotal;
    }
    else {
        top = vtop;
        total = vtotal;
    }

    trmpos(1, 1);
    trmrev();
    if(vmode)
        puts(" FILEEDIT  ");
    else
        puts(" FILEEDIT VIEWER  ");
    puts(fsname);
    if(vmode) {
        if(fsexist == 0)
            puts("  [NEW]");
        if(ebdirty)
            puts("  [MOD]");
    }
    puts(" ");
    trmesc("[K");
    trmnorm();

    trmrule(2);

    row = 3;
    shown = 0;
    while(row <= 22) {
        if(vline(row, top + shown) == 0)
            row = 23;
        else {
            shown = shown + 1;
            row = row + 1;
        }
    }

    row = 3 + shown;
    while(row <= 22) {
        trmpos(row, 1);
        trmclrln();
        row = row + 1;
    }

    last = top + shown - 1;
    if(shown == 0)
        last = top;
    if(last > total)
        last = total;

    trmrule(23);

    trmpos(24, 1);
    trmrev();

    if(vmode) {
        puts(" Ln ");
        trmnum(eline);
        puts("/");
        trmnum(etotal);
        puts(" Col ");
        trmnum(edvcol());
        trmpos(24, 27);
        puts("Arrows Home End PgUp/PgDn Tab  ^O Save  ^X Menu ");
    }
    else {
        if(vtotal == 0)
            puts(" Empty file  ");
        else {
            puts(" Lines ");
            trmnum(vtop);
            puts("-");
            trmnum(last);
            puts(" of ");
            trmnum(vtotal);
            puts("  ");
        }
        puts("Up/Down  PgUp/PgDn  Home  Q quit ");
    }

    trmesc("[K");
    trmnorm();
}
