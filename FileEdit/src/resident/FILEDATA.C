/*
** FILEDATA.C -- FILESTAT resident statistics/document storage (Step 6).
**
** The current editor filename is now resident writable storage rather than a
** pointer into argv.  Open/New/Save As may therefore change document identity
** without restarting FILESTAT.  All external identifiers remain eight
** characters or fewer for Small-C/Plus.
*/

extern puts();
extern putchar();

char fsfile[16];
char *fsname;
int fsunit;
int dolst;
char fsbak[16];
int fsexist;

int nline;
int nword;
int nchar;
int nalpha;
int ndigit;
int nspace;
int npunct;
int longlen;
char longword[32];

/* Copy a DOS/BDOS 8.3-style name into stable resident storage. */
fsset(p)
char *p;
{
    int i;

    fsname = fsfile;
    i = 0;
    while(p[i] != 0 && i < 15) {
        fsfile[i] = p[i];
        i = i + 1;
    }
    fsfile[i] = 0;
    return i;
}

/* Case-insensitive filename comparison used by Save As. */
fsequal(a, b)
char *a, *b;
{
    int ca;
    int cb;
    int i;

    i = 0;
    while(a[i] != 0 || b[i] != 0) {
        ca = a[i];
        cb = b[i];

        if(ca >= 'a' && ca <= 'z')
            ca = ca - 'a' + 'A';
        if(cb >= 'a' && cb <= 'z')
            cb = cb - 'a' + 'A';

        if(ca != cb)
            return 0;
        i = i + 1;
    }

    return 1;
}

fsreset()
{
    nline = 0;
    nword = 0;
    nchar = 0;
    nalpha = 0;
    ndigit = 0;
    nspace = 0;
    npunct = 0;
    longlen = 0;
    longword[0] = 0;
}

/* Save a new longest word.  Display text is limited to 31 characters. */
noteword(buf, len)
char *buf;
int len;
{
    int i;
    int n;

    if(len <= longlen)
        return;

    longlen = len;
    n = len;
    if(n > 31)
        n = 31;

    i = 0;
    while(i < n) {
        longword[i] = buf[i];
        i = i + 1;
    }
    longword[i] = 0;
}

/* Small decimal printer used by the report overlay. */
putnum(n)
int n;
{
    char buf[7];
    int i;
    int d;

    if(n == 0) {
        putchar('0');
        return;
    }

    if(n < 0) {
        putchar('-');
        n = -n;
    }

    i = 0;
    while(n && i < 6) {
        d = n % 10;
        buf[i] = d + '0';
        i = i + 1;
        n = n / 10;
    }

    while(i) {
        i = i - 1;
        putchar(buf[i]);
    }
}

putstat(name, value)
char *name;
int value;
{
    puts(name);
    puts(": ");
    putnum(value);
    puts("\n");
}
