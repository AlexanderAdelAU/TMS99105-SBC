/*
** OVL_EDIT.C -- FileEdit Step 7 one-key editor operation overlay.
**
** This overlay owns no persistent state.  All cursor/document variables live
** resident, and this module processes exactly one terminal key then returns.
** OVL_VIEW is mapped separately by the small resident EDITCTL controller.
*/
#asm
        AORG    08000H
#endasm

extern puts();
extern fssave();
extern trmkey();
extern trmshow();
extern trmhide();
extern trmclear();
extern trmpos();
extern trmrev();
extern trmnorm();
extern trmnum();
extern trmbell();
extern ebllen();
extern ebindex();
extern ebins();
extern ebdel();
extern edxcol();
extern edvcol();

extern int vleft;
extern int eblen;
extern char *ebbase;
extern char *fsname;

extern int etop;
extern int eline;
extern int ecol;
extern int ewant;
extern int etotal;
extern int eleft;
extern int edflag;

/* Clamp a line number to the editor's logical range. */
eclamp(n)
int n;
{
    if(n < 1) n = 1;
    if(n > etotal) n = etotal;
    return n;
}

/* Clamp actual column to the length of a line. */
eclmcol(line, col)
int line, col;
{
    int len;
    len = ebllen(line);
    if(col < 0) col = 0;
    if(col > len) col = len;
    return col;
}

/* Keep cursor line inside the 20-row viewport. */
evis()
{
    int max;
    if(eline < etop) etop = eline;
    if(eline > etop + 19) etop = eline - 19;
    max = etotal - 19;
    if(max < 1) max = 1;
    if(etop < 1) etop = 1;
    if(etop > max) etop = max;
}

/* Keep logical cursor inside the 73-column text viewport (cols 7..79). */
eadj()
{
    if(ecol < eleft)
        eleft = ecol;
    while(eleft < ecol && edxcol() > 73)
        eleft = eleft + 1;
    if(eleft < 0)
        eleft = 0;
    vleft = eleft;
}

/* Small status refresh used for cursor-only moves. */
edstat()
{
    trmpos(24, 1);
    trmrev();
    puts("                         ");
    trmpos(24, 1);
    puts(" Ln ");
    trmnum(eline);
    puts("/");
    trmnum(etotal);
    puts(" Col ");
    trmnum(edvcol());
    trmnorm();
}

/* Decode one complete VT100 key sequence. */
edkey()
{
    int c;
    int d;
    int par;
    int have;
    int n;

    c = trmkey();
    if(c == 24) return 9;       /* Ctrl-X */
    if(c == 15) return 13;      /* Ctrl-O */
    if(c == 9) return 14;       /* TAB */
    if(c == 8) return 10;       /* Backspace */
    if(c == 127) return 12;     /* Delete */
    if(c == 13 || c == 10) return 11;
    if(c >= 32 && c < 127) return c;
    if(c != 27) return 0;

    c = trmkey();
    if(c == 'O') {
        d = trmkey();
        if(d == 'A') return 1;
        if(d == 'B') return 2;
        if(d == 'D') return 3;
        if(d == 'C') return 4;
        if(d == 'H') return 7;
        if(d == 'F') return 8;
        return 0;
    }
    if(c != '[') return 0;

    par = 0;
    have = 0;
    n = 0;
    d = 0;
    while(n < 12) {
        c = trmkey();
        if(c >= '0' && c <= '9') {
            if(have == 0) par = 0;
            if(have < 2) par = (par * 10) + (c - '0');
            have = have + 1;
        }
        else if(c == ';') {
            if(have < 2) have = 2;
        }
        else if(c >= '@' && c <= '~') {
            d = c;
            n = 12;
        }
        n = n + 1;
    }

    if(d == 'A') return 1;
    if(d == 'B') return 2;
    if(d == 'D') return 3;
    if(d == 'C') return 4;
    if(d == 'H') return 7;
    if(d == 'F') return 8;
    if(d == 'Z') return 15;
    if(d == '~') {
        if(par == 1 || par == 7) return 7;
        if(par == 3) return 12;
        if(par == 4 || par == 8) return 8;
        if(par == 5) return 5;
        if(par == 6) return 6;
    }
    return 0;
}

emovev(delta)
int delta;
{
    eline = eclamp(eline + delta);
    ecol = eclmcol(eline, ewant);
    evis();
}

emovel()
{
    int len;
    if(ecol > 0)
        ecol = ecol - 1;
    else if(eline > 1) {
        eline = eline - 1;
        len = ebllen(eline);
        ecol = len;
    }
    ewant = ecol;
    evis();
}

emover()
{
    int len;
    len = ebllen(eline);
    if(ecol < len)
        ecol = ecol + 1;
    else if(eline < etotal) {
        eline = eline + 1;
        ecol = 0;
    }
    ewant = ecol;
    evis();
}

edins(c)
int c;
{
    int pos;
    pos = ebindex(eline, ecol);
    if(ebins(pos, c) == 0) {
        trmbell();
        return;
    }
    ecol = ecol + 1;
    ewant = ecol;
}

edenter()
{
    int pos;
    pos = ebindex(eline, ecol);
    if(ebins(pos, 13) == 0) {
        trmbell();
        return;
    }
    etotal = etotal + 1;
    eline = eline + 1;
    ecol = 0;
    ewant = 0;
    evis();
}

edbksp()
{
    int pos;
    int len;
    if(ecol > 0) {
        pos = ebindex(eline, ecol);
        if(ebdel(pos - 1) >= 0) {
            ecol = ecol - 1;
            ewant = ecol;
        }
    }
    else if(eline > 1) {
        len = ebllen(eline - 1);
        pos = ebindex(eline, 0);
        if(ebdel(pos - 1) >= 0) {
            eline = eline - 1;
            etotal = etotal - 1;
            if(etotal < 1) etotal = 1;
            ecol = len;
            ewant = ecol;
        }
    }
    evis();
}

eddel()
{
    int pos;
    int c;
    pos = ebindex(eline, ecol);
    c = ebdel(pos);
    if(c == 13) {
        etotal = etotal - 1;
        if(etotal < 1) etotal = 1;
    }
    ecol = eclmcol(eline, ecol);
    ewant = ecol;
    evis();
}

edsave()
{
    int ok;
    trmshow();
    trmclear();
    puts("Saving ");
    puts(fsname);
    puts(" ...\n");
    ok = fssave();
    if(ok == 0) {
        puts("\nSave did not complete.  RAM edits are still intact.\n");
        puts("Press any key to return to the editor...");
        trmkey();
    }
    trmclear();
    trmhide();
    return ok;
}

/* Process exactly one key and set resident edflag: 0=no redraw,1=redraw,2=done. */
editstep()
{
    int key;
    int row;
    int col;
    int oldtop;
    int oldleft;
    int redraw;

    eadj();
    edstat();
    row = 3 + (eline - etop);
    col = edxcol();
    if(col > 73) col = 73;
    trmpos(row, col + 6);
    trmshow();
    key = edkey();
    trmhide();

    oldtop = etop;
    oldleft = eleft;
    redraw = 0;

    if(key == 1) emovev(-1);
    else if(key == 2) emovev(1);
    else if(key == 3) emovel();
    else if(key == 4) emover();
    else if(key == 5) emovev(-20);
    else if(key == 6) emovev(20);
    else if(key == 7) { ecol = 0; ewant = 0; }
    else if(key == 8) { ecol = ebllen(eline); ewant = ecol; }
    else if(key == 9) { edflag = 2; return; }
    else if(key == 10) { edbksp(); redraw = 1; }
    else if(key == 11) { edenter(); redraw = 1; }
    else if(key == 12) { eddel(); redraw = 1; }
    else if(key == 13) { edsave(); redraw = 1; }
    else if(key == 14) { edins(9); redraw = 1; }
    else if(key >= 32 && key < 127) { edins(key); redraw = 1; }

    eadj();
    if(redraw == 0 && (etop != oldtop || eleft != oldleft))
        redraw = 1;
    edflag = redraw;
}
