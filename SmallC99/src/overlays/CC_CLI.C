/*
** CC_CLI.C -- M36a r3d one-shot command-line parser overlay
**
** AORG >A000, overlay ID 7, physical page 12. This module exists to
** keep command parsing and its messages out of common resident memory.
** It runs once through resident CLISTUB.R99, fills resident srcname /
** outname / verbose, returns, and is not needed during compilation.
*/

#asm
        AORG    0A000H
#endasm

#define YES 1
#define NO  0
#define NAMEMAX 14

extern puts();
extern char srcname[];
extern char outname[];
extern int verbose;

upcase(c)
int c;
{
    if(c >= 'a') {
        if(c <= 'z') return c - 32;
    }
    return c;
}

copyname(dst, src)
char *dst;
char *src;
{
    int n;

    n = 0;
    while(*src) {
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
    while(*src) {
        if(*src == '.') break;
        if(n >= NAMEMAX) return NO;
        dst[n++] = *src++;
    }
    dst[n] = 0;
    return appendext(dst, ".ASM");
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
    if(s[2]) return NO;
    return YES;
}

isverbose(s)
char *s;
{
    if(s[0] != '/') {
        if(s[0] != '-') return NO;
    }
    if(upcase(s[1]) != 'V') return NO;
    if(s[2]) return NO;
    return YES;
}

usage()
{
    puts("SmallC99 2.2 M37a\n");
    puts("Usage: SMALLC99 source[.C] [output[.ASM]] [/V]\n");
    puts("       SMALLC99 /?\n");
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
    srcname[0] = 0;
    outname[0] = 0;

    /* argv[0] is the executable name in the Small-C runtime. */
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
        if(appendext(outname, ".ASM") == NO) {
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
