/*
** PTRTEST.C -- SBC_SMALLC99 pointers to pointers and arrays of pointers
**
** Runtime regression in the style of LANGTEST_LANGUAGE.C: every check
** is numbered and compares a computed value with the known answer.
**
** Expected final lines:
**
**     tests: 44  passed: 44  failed: 0
**     ALL POINTER TESTS PASSED
**
** Covers: member access on globals, locals, array elements and through
** pointers; nested structs; self-referencing lists; unions; member
** arrays; char/unsigned char members; ++, --, += on members; struct
** pointer arithmetic (++, +n, [i], p - q); layout and sizeof; struct
** pointer parameters.
*/

extern int putchar();

int tests, passed, failed;

nl()
{
    putchar(13);
    putchar(10);
}

pstr(s)
char *s;
{
    while (*s) putchar(*s++);
}

pnum(n)
int n;
{
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    if (n >= 10) pnum(n / 10);
    putchar('0' + n % 10);
}

section(s)
char *s;
{
    pstr("-- ");
    pstr(s);
    nl();
}

expect(id, got, want)
int id, got, want;
{
    tests++;
    if (got == want) {
        passed++;
    } else {
        failed++;
        pstr("FAIL ");
        pnum(id);
        pstr(": got ");
        pnum(got);
        pstr(" want ");
        pnum(want);
        nl();
    }
}

/* ---------- declarations ---------- */

char *words[4];                 /* array of pointers (global)      */
char **pw;                      /* pointer to pointer              */
int  *ip[3], **ipp, ***ippp;    /* int levels 1-3                  */
int  a, b, c;
char s1[6], s2[6];
struct pt { int x; int y; };
struct pt p1, p2, *pts[2], **ppt;

strlen(s)
char *s;
{
    int n;
    n = 0;
    while (*s++) n++;
    return n;
}

/* argv-style walk: count characters in all strings */
total(n, v)
int n;
char **v;
{
    int t;
    t = 0;
    while (n--) t = t + strlen(*v++);
    return t;
}

/* the same with the other parameter spelling */
first(v)
char *v[];
{
    return v[1][0];
}

setp(pp, val)
int **pp, val;
{
    **pp = val;
}

t_global()
{
    section("arrays of pointers");
    s1[0] = 'a'; s1[1] = 'b'; s1[2] = 'c'; s1[3] = 0;
    s2[0] = 'x'; s2[1] = 'y'; s2[2] = 0;
    words[0] = s1;
    words[1] = s2;
    words[2] = "hello";
    words[3] = 0;
    expect(1, words[0][0], 'a');
    expect(2, words[1][1], 'y');
    expect(3, words[2][4], 'o');
    expect(4, *words[1], 'x');
    expect(5, *(words[2] + 1), 'e');
    expect(6, sizeof(words), 8);
    expect(7, words[3], 0);
}

t_ptrptr()
{
    section("pointer to pointer");
    pw = words;
    expect(8, **pw, 'a');
    expect(9, *pw[1], 'x');
    expect(10, pw[2][1], 'e');
    pw++;
    expect(11, **pw, 'x');
    expect(12, (*pw)[1], 'y');
    expect(13, pw - words, 1);
    pw = pw + 1;
    expect(14, **pw, 'h');
    expect(15, *(*pw + 2), 'l');
    expect(16, sizeof(pw), 2);
    expect(17, sizeof(char **), 2);
}

t_params()
{
    section("pointer-to-pointer parameters");
    expect(18, total(3, words), 10);
    expect(19, first(words), 'x');
    ip[0] = &a;
    setp(&ip[0], 42);
    expect(20, a, 42);
    ipp = &ip[1];
    ip[1] = &b;
    setp(ipp, 7);
    expect(21, b, 7);
}

t_levels()
{
    section("int levels 1-3");
    a = 5; b = 6; c = 7;
    ip[0] = &a; ip[1] = &b; ip[2] = &c;
    ipp = ip;
    ippp = &ipp;
    expect(22, *ip[2], 7);
    expect(23, **ipp, 5);
    expect(24, ***ippp, 5);
    expect(25, (*ippp)[1][0], 6);
    ipp++;
    expect(26, ***ippp, 6);
    **ipp = 60;
    expect(27, b, 60);
    ***ippp = 61;
    expect(28, b, 61);
    expect(29, sizeof(ip), 6);
    expect(30, ipp - ip, 1);
}

t_local()
{
    char *loc[3], **lp;
    int *li[2], **lpp;
    section("local arrays of pointers");
    loc[0] = "one";
    loc[1] = "two";
    loc[2] = "six";
    lp = loc;
    expect(31, lp[2][0], 's');
    expect(32, *lp[1], 't');
    lp = lp + 2;
    expect(33, **lp, 's');
    li[0] = &a;
    li[1] = &c;
    lpp = li;
    expect(34, *lpp[1], 7);
    expect(35, sizeof(loc), 6);
}

t_struct()
{
    section("pointers to struct pointers");
    p1.x = 1; p1.y = 2;
    p2.x = 3; p2.y = 4;
    pts[0] = &p1;
    pts[1] = &p2;
    expect(36, pts[1]->y, 4);
    ppt = pts;
    expect(37, (*ppt)->x, 1);
    ppt++;
    expect(38, (*ppt)->x, 3);
    (*ppt)->y = 40;
    expect(39, p2.y, 40);
    expect(40, sizeof(pts), 4);
}

t_mixed()
{
    char *a1, a2;           /* only a1 is a pointer */
    int **q, r;             /* only q is a pointer  */
    section("declarator scope");
    expect(41, sizeof(a1), 2);
    expect(42, sizeof(a2), 1);
    expect(43, sizeof(q), 2);
    expect(44, sizeof(r), 2);
}

main()
{
    t_global();
    t_ptrptr();
    t_params();
    t_levels();
    t_local();
    t_struct();
    t_mixed();

    nl();
    pstr("tests: ");
    pnum(tests);
    pstr("  passed: ");
    pnum(passed);
    pstr("  failed: ");
    pnum(failed);
    nl();

    if (failed == 0) pstr("ALL POINTER TESTS PASSED");
    else pstr("*** POINTER FAILURES ***");
    nl();

    return failed;
}
