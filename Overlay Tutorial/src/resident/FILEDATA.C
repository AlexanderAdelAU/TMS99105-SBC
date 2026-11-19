/*
** FILEDATA.C -- FILESTAT resident statistics storage and report helpers.
**
** These values must survive overlay changes, so they are kept in the
** resident part of the program rather than in an overlay.
*/

extern puts();
extern putchar();

char *fsname;
int fsunit;
int dolst;

int nline;
int nword;
int nchar;
int nalpha;
int ndigit;
int nspace;
int npunct;
int longlen;
char longword[32];

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
