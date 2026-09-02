/*
** VIEWCTL.C -- resident read-only viewer controller (Step 4).
**
** Step 4 no longer rewinds/re-reads the disk file on each redraw.  EDITBUF
** loads the file once into >9000-BFFF; this controller navigates that stable
** memory image while OVL_VIEW continues to render one frame at >8000.
*/

extern ebseek();
extern runview();
extern trmkey();
extern trmshow();
extern trmhide();
extern trmclear();
extern int eblines;

/* Persistent viewer state consumed by OVL_VIEW. */
int vtop;
int vtotal;
int vmode;
int vleft;

vclamp(top, total)
int top, total;
{
    int max;

    max = total - 19;
    if(max < 1)
        max = 1;

    if(top < 1)
        top = 1;
    if(top > max)
        top = max;

    return top;
}

/*
** Decode one Tera Term key while executing resident code.
** Return: 1 up, 2 down, 3 page up, 4 page down, 5 home, 9 quit, 0 ignore.
*/
vkey()
{
    int c;
    int d;

    c = trmkey();

    if(c == 'Q' || c == 'q')
        return 9;
    if(c == 'U' || c == 'u')
        return 1;
    if(c == 'D' || c == 'd')
        return 2;
    if(c == 'P' || c == 'p')
        return 3;
    if(c == 'N' || c == 'n')
        return 4;
    if(c == 'H' || c == 'h')
        return 5;

    if(c != 27)
        return 0;

    c = trmkey();
    if(c == 'O') {
        d = trmkey();
        if(d == 'A')
            return 1;
        if(d == 'B')
            return 2;
        if(d == 'H')
            return 5;
        return 0;
    }

    if(c != '[')
        return 0;

    c = trmkey();
    if(c == 'A')
        return 1;
    if(c == 'B')
        return 2;
    if(c == 'H')
        return 5;

    if(c == '1') {
        d = trmkey();
        if(d == '~')
            return 5;
    }

    if(c == '5') {
        d = trmkey();
        if(d == '~')
            return 3;
    }

    if(c == '6') {
        d = trmkey();
        if(d == '~')
            return 4;
    }

    return 0;
}

/* Resident lifetime controller for the read-only memory-buffer viewer. */
viewctl()
{
    int key;
    int done;

    vtotal = eblines;
    vtop = 1;
    vleft = 0;
    vmode = 0;
    done = 0;

    trmclear();
    trmhide();

    while(done == 0) {
        ebseek(vtop);
        runview();
        key = vkey();

        if(key == 1)
            vtop = vclamp(vtop - 1, vtotal);
        else if(key == 2)
            vtop = vclamp(vtop + 1, vtotal);
        else if(key == 3)
            vtop = vclamp(vtop - 20, vtotal);
        else if(key == 4)
            vtop = vclamp(vtop + 20, vtotal);
        else if(key == 5)
            vtop = 1;
        else if(key == 9)
            done = 1;
    }

    trmshow();
    trmclear();
}
