/*
** STRUCTTEST.C -- SBC_SMALLC99 structures, phase 1
**
** Runtime regression in the style of LANGTEST_LANGUAGE.C: every check
** is numbered and compares a computed value with the known answer.
**
** Expected final lines:
**
**     tests: 60  passed: 60  failed: 0
**     ALL STRUCT TESTS PASSED
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

struct pt { int x; int y; };

struct rec {
    char tag;               /* offset 0                         */
    int val;                /* offset 2 (aligned)               */
    char name[5];           /* offset 4                         */
    struct pt p;            /* offset 10 (aligned)              */
    struct rec *next;       /* offset 14; size 16               */
};

union u { char c; int i; char b[5]; };      /* size 6 */

struct node { int v; struct node *next; };

struct bytes { unsigned char uc; char sc; };

struct pt a, *pa, arr[3];
struct rec r;
union u uu;
struct node n1, n2, n3;
struct bytes by;

/* ---------- helpers taking struct pointers ---------- */

weigh(p)
struct pt *p;
{
    return p->x * 10 + p->y;
}

setpt(p, x, y)
struct pt *p;
int x, y;
{
    p->x = x;
    p->y = y;
}

sumlist(p)
struct node *p;
{
    int s;
    s = 0;
    while (p) {
        s = s + p->v;
        p = p->next;
    }
    return s;
}

/* ---------- tests ---------- */

t_global()
{
    section("global members");
    a.x = 3;
    a.y = 4;
    expect(1, a.x, 3);
    expect(2, a.y, 4);
    expect(3, a.x + a.y, 7);
    r.tag = 'A';
    r.val = 1000;
    expect(4, r.tag, 'A');
    expect(5, r.val, 1000);
    expect(6, r.tag + 1, 'B');
}

t_nested()
{
    section("nested struct and member arrays");
    r.p.x = 11;
    r.p.y = 22;
    expect(7, r.p.x, 11);
    expect(8, r.p.y, 22);
    r.name[0] = 'h';
    r.name[1] = 'i';
    r.name[4] = 'z';
    expect(9, r.name[0], 'h');
    expect(10, r.name[1], 'i');
    expect(11, r.name[4], 'z');
    expect(12, r.val, 1000);            /* neighbours untouched */
    expect(13, r.p.x, 11);
}

t_pointer()
{
    char *cp;
    section("pointers to structs");
    pa = &a;
    pa->x = 5;
    expect(14, a.x, 5);
    expect(15, pa->y, 4);
    expect(16, (*pa).x, 5);
    r.next = &r;
    expect(17, r.next->val, 1000);
    expect(18, r.next->p.y, 22);
    cp = r.name;
    expect(19, cp[4], 'z');
    cp = &r.p.y;
    expect(20, *cp == 0 || *cp == 22, 1);   /* high byte first */
}

t_array()
{
    int i, s;
    section("arrays of structs");
    i = 0;
    while (i < 3) {
        arr[i].x = i * 10;
        arr[i].y = i * 10 + 1;
        i++;
    }
    expect(21, arr[0].x, 0);
    expect(22, arr[1].y, 11);
    expect(23, arr[2].x, 20);
    expect(24, arr[2].y, 21);
    s = 0;
    i = 0;
    while (i < 3) s = s + arr[i++].y;
    expect(25, s, 33);
    expect(26, arr[1].x + arr[2].x, 30);
}

t_arith()
{
    struct pt *p, *q;
    section("struct pointer arithmetic");
    p = arr;
    expect(27, p->x, 0);
    p++;
    expect(28, p->x, 10);
    expect(29, (p + 1)->y, 21);
    expect(30, p[1].x, 20);
    expect(31, p[-1].y, 1);
    q = &arr[2];
    expect(32, q - arr, 2);
    expect(33, q - p, 1);
    p--;
    expect(34, p->y, 1);
    p = p + 2;
    expect(35, p->x, 20);
}

t_list()
{
    section("self-referencing list");
    n1.v = 1;
    n2.v = 20;
    n3.v = 300;
    n1.next = &n2;
    n2.next = &n3;
    n3.next = 0;
    expect(36, sumlist(&n1), 321);
    expect(37, n1.next->next->v, 300);
    expect(38, n1.next->next->next, 0);
}

t_union()
{
    section("unions");
    uu.i = 4660;                        /* >1234 */
    expect(39, uu.c, 18);               /* >12: high byte first */
    expect(40, uu.b[1], 52);            /* >34 */
    uu.c = 1;
    expect(41, uu.i, 308);              /* >0134 */
    expect(42, sizeof(uu), 6);
}

t_local()
{
    struct pt l, *lp, lar[2];
    section("local structs");
    l.x = 7;
    l.y = 8;
    expect(43, l.x + l.y, 15);
    lp = &l;
    lp->y = 9;
    expect(44, l.y, 9);
    lar[1].x = 30;
    lar[0].x = 40;
    expect(45, lar[1].x, 30);
    expect(46, lar[0].x, 40);
    setpt(&lar[1], 2, 3);
    expect(47, weigh(&lar[1]), 23);
    expect(48, weigh(lp), 79);
}

t_ops()
{
    section("operators on members");
    a.x = 10;
    a.x++;
    expect(49, a.x, 11);
    ++a.y;
    expect(50, a.y, 5);
    pa = &a;
    pa->x += 5;
    expect(51, a.x, 16);
    pa->y--;
    expect(52, pa->y, 4);
    by.uc = 200;
    by.sc = 200;
    expect(53, by.uc, 200);
    expect(54, by.sc, -56);
}

t_layout()
{
    int base, at;
    section("layout and sizeof");
    expect(55, sizeof(struct pt), 4);
    expect(56, sizeof(struct rec), 16);
    expect(57, sizeof(arr), 12);
    base = &r;                          /* addresses as ints: bytes */
    at = &r.val;
    expect(58, at - base, 2);
    at = &r.p;
    expect(59, at - base, 10);
    at = &r.next;
    expect(60, at - base, 14);
}

main()
{
    t_global();
    t_nested();
    t_pointer();
    t_array();
    t_arith();
    t_list();
    t_union();
    t_local();
    t_ops();
    t_layout();

    nl();
    pstr("tests: ");
    pnum(tests);
    pstr("  passed: ");
    pnum(passed);
    pstr("  failed: ");
    pnum(failed);
    nl();

    if (failed == 0) pstr("ALL STRUCT TESTS PASSED");
    else pstr("*** STRUCT FAILURES ***");
    nl();

    return failed;
}
