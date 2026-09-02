/*
** TERMIO.C -- resident ANSI/VT100 terminal services for Tera Term.
**
** Step 3 adds cursor positioning and direct/no-echo keyboard input.  These
** routines stay resident because every display/editor overlay will share
** them.  External identifiers remain eight characters or fewer for the
** Small-C/Plus toolchain.
*/

extern putchar();
extern puts();
extern rxwait();
extern int rxchar;

/* Emit ESC followed by the supplied ANSI control sequence. */
trmesc(s)
char *s;
{
    putchar(27);
    puts(s);
}

/*
** Clear the display using the sequence proven by ED2 on VT100/Tera Term:
** HOME first, then erase from HOME to end of display.  This avoids the
** repeated-screen/scrollback behaviour seen with CSI 2 J on this setup.
*/
trmclear()
{
    trmesc("[H");
    trmesc("[J");
}

/* Reverse video is useful for simple title and status bars. */
trmrev()
{
    trmesc("[7m");
}

/* Restore normal terminal attributes. */
trmnorm()
{
    trmesc("[0m");
}

/* Hide/show the hardware cursor while a screen-oriented view is active. */
trmhide()
{
    trmesc("[?25l");
}

trmshow()
{
    trmesc("[?25h");
}

/* Audible/visible terminal bell for a rejected edit (for example full buffer). */
trmbell()
{
    putchar(7);
}

/* Print a positive decimal integer without pulling printf into TERMIO. */
trmnum(n)
int n;
{
    if(n < 0) {
        putchar('-');
        n = -n;
    }

    if(n >= 10)
        trmnum(n / 10);

    putchar('0' + (n % 10));
}

/*
** ANSI cursor position: rows/columns are one-origin values.
** ED2 emits fixed two-digit coordinates (ESC[03;01H), so do the same here.
** The editor is deliberately fixed at 80x24 for now; values above 99 are not
** needed at this milestone.
*/
trmpos(row, col)
int row, col;
{
    putchar(27);
    putchar('[');
    putchar('0' + ((row / 10) % 10));
    putchar('0' + (row % 10));
    putchar(';');
    putchar('0' + ((col / 10) % 10));
    putchar('0' + (col % 10));
    putchar('H');
}

/* Erase from column 1 to end of line; callers position at column 1 first. */
trmclrln()
{
    trmesc("[K");
}

/*
** Blocking, no-echo console input from the resident INT4 RX queue.
**
** FileEdit Step 7.1 installs a temporary INT4 hook while /E or /V is
** active.  The ISR drains TMS9902 receive bytes immediately into a 64-byte
** ring, so full-screen redraws no longer leave ESC-sequence tails behind.
*/
trmkey()
{
    rxwait();
    return rxchar;
}

/* Retained as an alias for later terminal code; currently identical. */
trmraw()
{
    return trmkey();
}

/* Draw a 79-column rule; leave column 80 unused to avoid auto-wrap. */
trmrule(row)
int row;
{
    int n;

    trmpos(row, 1);
    trmclrln();
    n = 0;
    while(n < 79) {
        putchar('-');
        n = n + 1;
    }
}
