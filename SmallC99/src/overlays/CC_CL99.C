/*
** CC_CL99.C -- M38d one-shot command-line parser for SMALLC99
**
** AORG >A000, overlay ID 7, physical page 12. The implementation is
** intentionally parallel to CC_CLI, but derives native .A99 output names
** and identifies the separate TMS-target compiler.
*/

#asm
        AORG    0A000H
#endasm

#define YES 1
#define NO  0
#define NAMEMAX 14

extern puts();
extern putchar();
extern char srcname[];
extern char outname[];
extern int verbose;
extern int mainflg;

upcase(c)
int c;
{
    if(c >= 'a') {
        if(c <= 'z') return c - 32;
    }
    return c;
}

/*  Names stop at any control character, not just NUL.  A command line
    handed over by the procedure engine still carries its CR, and
    _setargs splits on SPACE only, so the last token arrives as
    "MAIN2\r" and appendext then builds "MAIN2\r.C".  */
copyname(dst, src)
char *dst;
char *src;
{
    int n;

    n = 0;
    while(*src > ' ') {
        if(n >= NAMEMAX) {
            dst[NAMEMAX] = 0;
            return NO;
        }
        dst[n++] = upcase(*src++);
    }
    dst[n] = 0;
    return YES;
}

hasdot(s)
char *s;
{
    while(*s) {
        if(*s == '.') return YES;
        ++s;
    }
    return NO;
}

appendext(s, ext)
char *s;
char *ext;
{
    int n;

    n = 0;
    while(s[n]) ++n;
    while(*ext) {
        if(n >= NAMEMAX) return NO;
        s[n++] = *ext++;
    }
    s[n] = 0;
    return YES;
}

deriveout(dst, src)
char *dst;
char *src;
{
    int n;

    n = 0;
    while(*src > ' ') {
        if(*src == '.') break;
        if(n >= NAMEMAX) return NO;
        dst[n++] = *src++;
    }
    dst[n] = 0;
    return appendext(dst, ".A99");
}

samefile(a, b)
char *a;
char *b;
{
    while(*a) {
        if(*a != *b) return NO;
        ++a;
        ++b;
    }
    if(*b) return NO;
    return YES;
}

ishelp(s)
char *s;
{
    if(s[0] != '/') {
        if(s[0] != '-') return NO;
    }
    if(s[1] != '?') return NO;
    if(s[2] > ' ') return NO;	/*  tolerate a trailing CR  */
    return YES;
}

isverbose(s)
char *s;
{
    if(s[0] != '/') {
        if(s[0] != '-') return NO;
    }
    if(upcase(s[1]) != 'V') return NO;
    if(s[2] > ' ') return NO;	/*  tolerate a trailing CR  */
    return YES;
}

ismodule(s)
char *s;
{
    if(s[0] != '/') {
        if(s[0] != '-') return NO;
    }
    if(upcase(s[1]) != 'M') return NO;
    if(s[2] > ' ') return NO;	/*  tolerate a trailing CR  */
    return YES;
}

usage()
{
    puts("Usage: SMALLC99 source[.C] [output[.A99]] [-V] [-M]\n");
    puts("       SMALLC99 /?\n");
    puts("  -V   verbose compile progress (/V also accepted)\n");
    puts("  -M   module mode: omit IOLIB startup branch (/M also accepted)\n");
    puts("  If output is omitted, source.A99 is used.\n");
}

badname(name)
char *name;
{
    puts("SMALLC99: filename is too long: ");
    puts(name);
    puts("\n");
}

getoptions(argc, argv)
int argc;
char **argv;
{
    int i;
    int files;
    char *arg;

    files = 0;
    verbose = NO;
    mainflg = YES;             /* normal executable unless -M is present */
    srcname[0] = 0;
    outname[0] = 0;

    i = 1;
    while(i < argc) {
        arg = argv[i++];
        if(ishelp(arg)) {
            usage();
            return NO;
        }
        if(isverbose(arg)) {
            verbose = YES;
            continue;
        }
        if(ismodule(arg)) {
            mainflg = NO;
            continue;
        }
        if(arg[0] == '/') {
            puts("SMALLC99: unknown switch: ");
            puts(arg);
            puts("\n");
            return NO;
        }
        if(arg[0] == '-') {
            puts("SMALLC99: unknown switch: ");
            puts(arg);
            puts("\n");
            return NO;
        }

        if(files == 0) {
            if(copyname(srcname, arg) == NO) {
                badname(arg);
                return NO;
            }
        }
        else if(files == 1) {
            if(copyname(outname, arg) == NO) {
                badname(arg);
                return NO;
            }
        }
        else {
            puts("SMALLC99: too many filenames\n");
            return NO;
        }
        ++files;
    }

    if(files == 0) {
        usage();
        return NO;
    }

    if(hasdot(srcname) == NO) {
        if(appendext(srcname, ".C") == NO) {
            badname(srcname);
            return NO;
        }
    }

    if(files == 1) {
        if(deriveout(outname, srcname) == NO) {
            badname(srcname);
            return NO;
        }
    }
    else if(hasdot(outname) == NO) {
        if(appendext(outname, ".A99") == NO) {
            badname(outname);
            return NO;
        }
    }

    if(samefile(srcname, outname)) {
        puts("SMALLC99: input and output names are identical\n");
        return NO;
    }

    return YES;
}

/*
** ==== Error reports (reached through R_ERROR; error() is resident) ====
**
** errrep(msg)  one error: console   *** line N: msg
**                                       <the source line>
**                          .A99     ;*** line N: msg
** errrep(0)    the summary at the end: SMALLC99: N error(s)
** An error inside an #include reports the #include line's number.
*/
extern int errcnt;
extern int lineno;
extern int incunit;
extern int outunit;
extern char *line;      /* the current line (raw or macro-expanded) */

putnum(n) int n; {
    char b[6];
    int k;
    k = 5;
    b[k] = 0;
    do {
        b[--k] = 48 + n % 10;
        n = n / 10;
    } while(n && k);
    puts(b + k);
}

filenum(n) int n; {
    char b[6];
    int k;
    k = 5;
    b[k] = 0;
    do {
        b[--k] = 48 + n % 10;
        n = n / 10;
    } while(n && k);
    pstr(b + k);
}

errrep(msg) char *msg; {
    char *p;
    if(msg == 0) {
        puts("SMALLC99: ");
        putnum(errcnt);
        puts(errcnt == 1 ? " error\n" : " errors\n");
        return;
    }
    puts("*** line ");
    putnum(lineno);
    if(incunit) puts(" (in include)");
    puts(": ");
    puts(msg);
    puts("\n    ");
    p = line;
    while(*p == ' ' || *p == 9) ++p;
    while(*p && *p != 13 && *p != 10) putchar(*p++);    /* up to its CR */
    puts("\n");
    if(outunit) {
        pnl();
        pstr(";*** line ");
        filenum(lineno);
        pstr(": ");
        pstr(msg);
        pnl();
    }
}
