/*
** FILEIO.C -- FILESTAT resident file services (Step 6).
**
** Step 6 generalises the hardware-proven Step 5 save path so it can save the
** current document or Save As to another filename.  Existing targets receive
** a rolling .BAK before replacement; every write is closed, reopened and
** verified before the editor marks the document clean.
*/

extern fopen();
extern fclose();
extern fgetc();
extern putc();
extern putchar();
extern puts();
extern fsset();
extern fsequal();

extern char *fsname;
extern char fsbak[16];
extern int fsunit;
extern int fsexist;
extern char *ebbase;
extern int eblen;
extern int ebdirty;
extern int ebsaved;

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

/* Return 1 when a readable file currently exists under name. */
fsprobe(name)
char *name;
{
    int in;

    in = fopen(name, "r");
    if(in == 0)
        return 0;
    fclose(in);
    return 1;
}

/*
** IOLIB normalizes a text line ending to Small-C '\n', which this compiler
** represents as CR (0x0D).  Tera Term already handles that logical newline.
*/
fsout(c)
int c;
{
    putchar(c);
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

/* Uppercase one ASCII character without adding a library dependency. */
upch(c)
int c;
{
    if(c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';
    return c;
}

/* Build a rolling backup filename for an arbitrary target. */
mkbak(name)
char *name;
{
    int i;
    int dot;
    int isbak;

    i = 0;
    dot = -1;
    while(name[i] != 0 && i < 14) {
        fsbak[i] = name[i];
        if(name[i] == '.')
            dot = i;
        i = i + 1;
    }
    fsbak[i] = 0;

    isbak = 0;
    if(dot >= 0) {
        if(name[dot + 1] != 0 &&
           name[dot + 2] != 0 &&
           name[dot + 3] != 0 &&
           upch(name[dot + 1]) == 'B' &&
           upch(name[dot + 2]) == 'A' &&
           upch(name[dot + 3]) == 'K' &&
           name[dot + 4] == 0)
            isbak = 1;
        i = dot + 1;
    }
    else {
        if(i < 14) {
            fsbak[i] = '.';
            i = i + 1;
        }
    }

    if(isbak) {
        fsbak[i] = 'B';
        fsbak[i + 1] = 'K';
        fsbak[i + 2] = '1';
    }
    else {
        fsbak[i] = 'B';
        fsbak[i + 1] = 'A';
        fsbak[i + 2] = 'K';
    }
    fsbak[i + 3] = 0;
}

/* Copy one existing target to the rolling backup. */
cpyback(name)
char *name;
{
    int in;
    int out;
    int c;
    int ok;

    in = fopen(name, "r");
    if(in == 0) {
        puts("SAVE: cannot reopen existing file for backup\n");
        return 0;
    }

    out = fopen(fsbak, "w");
    if(out == 0) {
        fclose(in);
        puts("SAVE: cannot create backup ");
        puts(fsbak);
        puts("\nExisting file was not changed.\n");
        return 0;
    }

    ok = 1;
    c = fgetc(in);
    while(c != -1 && ok) {
        if(c != 10) {
            if(putc(c, out) == -1)
                ok = 0;
        }
        c = fgetc(in);
    }

    if(ok) {
        if(putc(26, out) == -1)
            ok = 0;
    }

    fclose(in);
    fclose(out);

    if(ok == 0) {
        puts("SAVE: backup write failed; existing file was not changed.\n");
        return 0;
    }

    return 1;
}

/* Write the persistent editor image to name. */
wrname(name)
char *name;
{
    int out;
    int i;
    int ok;

    out = fopen(name, "w");
    if(out == 0) {
        puts("SAVE: cannot create/replace ");
        puts(name);
        puts("\n");
        return 0;
    }

    ok = 1;
    i = 0;
    while(i < eblen && ok) {
        if(putc(ebbase[i], out) == -1)
            ok = 0;
        i = i + 1;
    }

    if(ok) {
        if(putc(26, out) == -1)
            ok = 0;
    }

    fclose(out);

    if(ok == 0) {
        puts("SAVE: disk write failed.  Edited RAM is still intact.\n");
        return 0;
    }

    return 1;
}

/* Re-open name and compare its normalized text stream to RAM. */
vrname(name)
char *name;
{
    int in;
    int i;
    int c;
    int ok;

    in = fopen(name, "r");
    if(in == 0) {
        puts("SAVE: file written but cannot be reopened for verify.\n");
        return 0;
    }

    i = 0;
    ok = 1;
    c = fgetc(in);

    while(c != -1 && ok) {
        if(c != 10) {
            if(i >= eblen)
                ok = 0;
            else if(c != ebbase[i])
                ok = 0;
            else
                i = i + 1;
        }
        c = fgetc(in);
    }

    if(i != eblen)
        ok = 0;

    fclose(in);

    if(ok == 0) {
        puts("SAVE: verification FAILED.  Edited RAM is still intact.\n");
        return 0;
    }

    return 1;
}

/* Save the current filename, creating a backup only when it already exists. */
fssave()
{
    if(fsexist && ebdirty == 0) {
        ebsaved = 1;
        return 1;
    }

    if(fsexist) {
        mkbak(fsname);
        if(cpyback(fsname) == 0)
            return 0;
    }

    if(wrname(fsname) == 0)
        return 0;

    if(vrname(fsname) == 0)
        return 0;

    fsexist = 1;
    ebdirty = 0;
    ebsaved = 1;
    return 1;
}

/*
** Save As to name.  The old document identity is retained unless the new
** target has been written and verified successfully.
*/
fssavas(name)
char *name;
{
    int exists;

    if(fsequal(fsname, name))
        return fssave();

    exists = fsprobe(name);
    if(exists) {
        mkbak(name);
        if(cpyback(name) == 0)
            return 0;
    }

    if(wrname(name) == 0)
        return 0;

    if(vrname(name) == 0)
        return 0;

    fsset(name);
    fsexist = 1;
    ebdirty = 0;
    ebsaved = 1;
    return 1;
}
