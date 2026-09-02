/*
** EDITBUF.C -- resident editor-buffer services (Step 6).
**
** The text arena remains >9000-BFFF and is still invisible to the user.  Step
** 6 adds persistent document lifecycle: safe load-by-name, New and a validity
** flag so Open failures never destroy the document already in memory.
*/

extern fopen();
extern fclose();
extern fgetc();
extern puts();
extern fsset();

extern char *fsname;
extern int fsexist;

char *ebbase;
int eblen;
int eblines;
int ebcap;
int vidx;
int ebdirty;
int ebsaved;
int ebvalid;

/* Initialise an empty, valid editor arena. */
ebnew()
{
    ebbase = 0x9000;
    ebcap = 12287;
    eblen = 0;
    eblines = 0;
    vidx = 0;
    ebdirty = 0;
    ebsaved = 0;
    ebvalid = 1;
    ebbase[0] = 0;
}

/* Initialise the arena when no document is currently open. */
ebnone()
{
    ebbase = 0x9000;
    ebcap = 12287;
    eblen = 0;
    eblines = 0;
    vidx = 0;
    ebdirty = 0;
    ebsaved = 0;
    ebvalid = 0;
    fsexist = 0;
    ebbase[0] = 0;
}

/* Start a new, empty named document.  Nothing is written to disk here. */
ebnewn(name)
char *name;
{
    ebnew();
    fsset(name);
    fsexist = 0;
    return 1;
}

/*
** Load name into the persistent arena without risking the current document.
**
** First pass: open/count only.  If the file is missing or too large, return
** with the existing RAM document untouched.  Second pass: open successfully,
** then reset/fill the arena and commit fsname only after the load completes.
*/
ebloadn(name)
char *name;
{
    int in;
    int c;
    int n;
    int last;

    in = fopen(name, "r");
    if(in == 0) {
        puts("FILESTAT: cannot open ");
        puts(name);
        puts("\n");
        return 0;
    }

    n = 0;
    c = fgetc(in);
    while(c != -1) {
        if(c != 10) {
            n = n + 1;
            if(n > 12287) {
                fclose(in);
                puts("FILESTAT: file is too large for this editor\n");
                return 0;
            }
        }
        c = fgetc(in);
    }
    fclose(in);

    in = fopen(name, "r");
    if(in == 0) {
        puts("FILESTAT: file could not be reopened\n");
        return 0;
    }

    ebnew();
    last = -1;
    c = fgetc(in);
    while(c != -1) {
        if(c != 10) {
            ebbase[eblen] = c;
            eblen = eblen + 1;
            if(c == 13)
                eblines = eblines + 1;
            last = c;
        }
        c = fgetc(in);
    }
    fclose(in);

    if(last != -1 && last != 13)
        eblines = eblines + 1;

    ebbase[eblen] = 0;
    vidx = 0;
    ebdirty = 0;
    ebsaved = 0;
    ebvalid = 1;
    fsset(name);
    fsexist = 1;
    return 1;
}

/* Compatibility wrapper: load the current resident filename. */
ebload()
{
    return ebloadn(fsname);
}

/* Return the physical file-line count used by the read-only viewer. */
ebtotln()
{
    if(eblines == 0)
        return 1;
    return eblines;
}

/*
** Return the editor cursor-line count.
** A terminal CR exposes one final empty editable line, just as a modern
** screen editor does.  The read-only FILESTAT line count remains unchanged.
*/
ebedln()
{
    if(eblen == 0)
        return 1;

    if(ebbase[eblen - 1] == 13)
        return eblines + 1;

    if(eblines == 0)
        return 1;

    return eblines;
}

/* Position the shared renderer index at the first byte of logical line n. */
ebseek(n)
int n;
{
    int i;
    int line;

    if(n < 1)
        n = 1;

    i = 0;
    line = 1;
    while(i < eblen && line < n) {
        if(ebbase[i] == 13)
            line = line + 1;
        i = i + 1;
    }

    vidx = i;
    return i;
}

/* Return the logical character length of line n, excluding its CR. */
ebllen(n)
int n;
{
    int i;
    int len;

    ebseek(n);
    i = vidx;
    len = 0;

    while(i < eblen && ebbase[i] != 13) {
        len = len + 1;
        i = i + 1;
    }

    return len;
}

/* Return the buffer index corresponding to a line/character column. */
ebindex(line, col)
int line, col;
{
    int i;

    ebseek(line);
    i = vidx;

    while(col > 0 && i < eblen && ebbase[i] != 13) {
        i = i + 1;
        col = col - 1;
    }

    return i;
}

/* Insert one byte at buffer index pos.  Return 1, or 0 when full. */
ebins(pos, c)
int pos, c;
{
    int i;

    if(pos < 0)
        pos = 0;
    if(pos > eblen)
        pos = eblen;

    if(eblen >= ebcap)
        return 0;

    i = eblen;
    while(i > pos) {
        ebbase[i] = ebbase[i - 1];
        i = i - 1;
    }

    ebbase[pos] = c;
    eblen = eblen + 1;
    ebbase[eblen] = 0;
    ebdirty = 1;
    return 1;
}

/* Delete one byte at buffer index pos.  Return deleted byte or -1. */
ebdel(pos)
int pos;
{
    int c;
    int i;

    if(pos < 0 || pos >= eblen)
        return -1;

    c = ebbase[pos];
    i = pos;
    while(i < eblen - 1) {
        ebbase[i] = ebbase[i + 1];
        i = i + 1;
    }

    eblen = eblen - 1;
    ebbase[eblen] = 0;
    ebdirty = 1;
    return c;
}
