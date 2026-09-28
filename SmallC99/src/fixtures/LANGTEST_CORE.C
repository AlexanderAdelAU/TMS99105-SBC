/*
 * LANGTEST_CORE.C
 *
 * Core language regression for SBC_SMALLC99 / TMS9900.
 * Purpose: test language semantics first, without deliberately stressing
 * parser recursion, compiler stack depth, or large temporary tables.
 *
 * Expected final line:
 *
 *     ALL CORE TESTS PASSED
 *
 * Written in the classic Small-C/K&R style used by this compiler.
 */

#define TEN 10
#define TWENTY (TEN * 2)
#define PPFLAG

#ifdef PPFLAG
#define IFVAL 11
#else
#define IFVAL 91
#endif

#ifndef NOT_DEFINED
#define IFNVAL 13
#else
#define IFNVAL 93
#endif

extern int putchar();

int tests, passed, failed;
int sidefx;
int zglob;
int zarr[4];
int gi;
char gc;
int garr[10];
char gbuf[32];

int ginit = 1234;
int gnums[4] = { 10, 20, 30, 40 };
char gmsg[] = "abc";
char *gptr = "xyz";
unsigned gu = 0xffff;
unsigned char guc = 255;

nl()
{
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
    if (n == -32767 - 1) {
        pstr("-32768");
        return;
    }
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

mystrlen(s)
char *s;
{
    int n;
    n = 0;
    while (*s++) n++;
    return n;
}

mystrcmp(a, b)
char *a, *b;
{
    while (*a == *b) {
        if (*a == 0) return 0;
        a++;
        b++;
    }
    return *a - *b;
}

mystrcpy(d, s)
char *d, *s;
{
    while (*d++ = *s++)
        ;
}

bump()
{
    sidefx++;
    return 1;
}

/* ---------- BSS and globals ---------- */

t_bss()
{
    section("bss/globals");
    expect(101, zglob, 0);
    expect(102, zarr[0], 0);
    expect(103, zarr[3], 0);
    expect(104, ginit, 1234);
    expect(105, gnums[0], 10);
    expect(106, gnums[3], 40);
    expect(107, gmsg[2], 'c');
    expect(108, gptr[0], 'x');
    expect(109, guc, 255);
}

/* ---------- constants and strings ---------- */

t_const()
{
    char *s;

    section("constants/strings");
    expect(201, 12345, 12345);
    expect(202, 'A', 65);
    expect(203, '\n', 10);
    expect(204, '\t', 9);
    expect(205, '\\', 92);
    expect(206, '\'', 39);
    expect(207, 0x10, 16);
    expect(208, 077, 63);

    s = "Hello";
    expect(209, s[0], 'H');
    expect(210, s[4], 'o');
    expect(211, s[5], 0);
    expect(212, mystrlen("abcdef"), 6);
    expect(213, mystrcmp("same", "same"), 0);
}

/* ---------- signed and unsigned arithmetic ---------- */

t_arith()
{
    int a, b;
    unsigned u;

    section("arithmetic");
    a = 7;
    b = 3;
    expect(301, a + b, 10);
    expect(302, a - b, 4);
    expect(303, a * b, 21);
    expect(304, a / b, 2);
    expect(305, a % b, 1);
    expect(306, -a, -7);
    expect(307, 2 + 3 * 4, 14);
    expect(308, (2 + 3) * 4, 20);
    expect(309, 10 - 4 - 3, 3);
    expect(310, 100 / 10 / 5, 2);

    u = 0xffff;
    expect(311, u >> 1, 0x7fff);
    expect(312, u / 2, 0x7fff);
    expect(313, u % 2, 1);
    u = 0x8000;
    expect(314, u > 0x7fff, 1);
    expect(315, u >> 1, 0x4000);

    u = 0xffff;
    u >>= 1;
    expect(316, u, 0x7fff);
    u = 0xffff;
    u /= 2;
    expect(317, u, 0x7fff);
    u = 0xffff;
    u %= 2;
    expect(318, u, 1);
    u = 0x8000;
    expect(319, u >> 0, 0x8000);
    b = 4;
    expect(320, u >> b, 0x0800);
    a = -16;
    expect(321, a >> 2, -4);
}

/* ---------- bitwise and relational ---------- */

t_bits_rel()
{
    int a, b, c;

    section("bitwise/relational");
    a = 12;
    b = 10;
    expect(401, a & b, 8);
    expect(402, a | b, 14);
    expect(403, a ^ b, 6);
    expect(404, ~0, -1);
    expect(405, 1 << 4, 16);
    expect(406, 256 >> 4, 16);
    expect(407, 1 | 2 & 4, 1);

    a = 3;
    b = 5;
    c = -2;
    expect(408, a < b, 1);
    expect(409, a > b, 0);
    expect(410, a <= 3, 1);
    expect(411, a >= 4, 0);
    expect(412, a == 3, 1);
    expect(413, a != 3, 0);
    expect(414, c < a, 1);
    expect(415, !0, 1);
    expect(416, !!a, 1);
}

/* ---------- logical and conditional ---------- */

t_logic()
{
    int a, b, x;

    section("logical/ternary");
    a = 0;
    b = 5;
    sidefx = 0;
    x = a && bump();
    expect(501, x, 0);
    expect(502, sidefx, 0);

    sidefx = 0;
    x = 1 || bump();
    expect(503, x, 1);
    expect(504, sidefx, 0);

    sidefx = 0;
    x = 1 && bump();
    expect(505, x, 1);
    expect(506, sidefx, 1);

    a = 5;
    expect(507, a > 3 ? 7 : 9, 7);
    expect(508, a < 3 ? 7 : 9, 9);
}

/* ---------- assignments and inc/dec ---------- */

t_assign()
{
    int a, b, c;

    section("assignment/inc/dec");
    a = b = c = 7;
    expect(601, a, 7);
    expect(602, b, 7);
    expect(603, c, 7);

    a = 5;
    b = a++;
    expect(604, b, 5);
    expect(605, a, 6);
    b = ++a;
    expect(606, b, 7);
    b = a--;
    expect(607, b, 7);
    b = --a;
    expect(608, b, 5);

    a = 10;
    a += 5; expect(609, a, 15);
    a -= 3; expect(610, a, 12);
    a *= 2; expect(611, a, 24);
    a /= 5; expect(612, a, 4);
    a %= 3; expect(613, a, 1);
    a <<= 4; expect(614, a, 16);
    a >>= 2; expect(615, a, 4);
    a |= 3; expect(616, a, 7);
    a &= 5; expect(617, a, 5);
    a ^= 1; expect(618, a, 4);
}

/* ---------- control flow ---------- */

sw(n)
int n;
{
    switch (n) {
    case 1:
        return 10;
    case 2:
    case 3:
        return 20;
    case -1:
        return 30;
    default:
        return 99;
    }
}

t_control()
{
    int a, b, i, j, s, x;

    section("if/else");
    x = 0;
    if (1) x = 1;
    expect(701, x, 1);
    if (0) x = 2; else x = 3;
    expect(702, x, 3);

    section("while");
    i = 1;
    s = 0;
    while (i <= 10) {
        s += i;
        i++;
    }
    expect(703, s, 55);

    section("for");
    s = 0;
    for (i = 0; i < 10; i++) s += i;
    expect(704, s, 45);

    section("do/while");
    i = 0;
    s = 0;
    do {
        i++;
        if (i == 3) continue;
        s++;
    } while (i < 5);
    expect(705, s, 4);

    s = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 5; j++) {
            if (j == 2) break;
            s++;
        }
    }
    expect(706, s, 8);

    section("switch/case/default");
    expect(707, sw(1), 10);
    expect(708, sw(2), 20);
    expect(709, sw(3), 20);
    expect(710, sw(-1), 30);
    expect(711, sw(7), 99);

    x = 0;
    a = 2;
    switch (a) {
    case 1: x += 1;
    case 2: x += 10;
    case 3: x += 100; break;
    case 4: x += 1000;
    }
    expect(712, x, 110);

    section("goto/labels");
    x = 1;
    goto reached;
    x = 99;
reached:
    x += 4;
    expect(713, x, 5);

    a = 1;
    b = 0;
    x = 0;
    if (a) if (b) x = 1; else x = 2;
    expect(714, x, 2);
}

/* ---------- functions ---------- */

add2(a, b)
int a, b;
{
    return a + b;
}

args6(a, b, c, d, e, f)
int a, b, c, d, e, f;
{
    return a + b * 2 + c * 3 + d * 4 + e * 5 + f * 6;
}

fact(n)
int n;
{
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

setvia(p, v)
int *p, v;
{
    *p = v;
}

incparam(n)
int n;
{
    n++;
    return n;
}

t_func()
{
    int a, v;

    section("functions");
    expect(801, add2(2, 3), 5);
    expect(802, args6(1, 2, 3, 4, 5, 6), 91);
    expect(803, fact(5), 120);
    setvia(&v, 77);
    expect(804, v, 77);
    a = 5;
    expect(805, incparam(a), 6);
    expect(806, a, 5);
    expect(807, add2(add2(1, 2), add2(3, 4)), 10);
}

/* ---------- pointers, arrays and chars ---------- */

sumarr(p, n)
int *p, n;
{
    int s;
    s = 0;
    while (n--) s += *p++;
    return s;
}

t_ptr_char()
{
    int a, b, i, *p, *q, arr[8];
    char c, d, *cp, buf[16];

    section("pointers/arrays/chars");
    a = 10;
    p = &a;
    expect(901, *p, 10);
    *p = 20;
    expect(902, a, 20);

    for (i = 0; i < 8; i++) arr[i] = i * i;
    expect(903, arr[7], 49);
    expect(904, sumarr(arr, 8), 140);
    p = arr;
    expect(905, *(p + 3), 9);
    expect(906, p[5], 25);
    q = &arr[7];
    expect(907, q - p, 7);
    expect(908, p < q, 1);

    b = *p++;
    expect(909, b, 0);
    expect(910, *p, 1);

    cp = buf;
    *cp++ = 'h';
    *cp++ = 'i';
    *cp = 0;
    expect(911, mystrlen(buf), 2);
    mystrcpy(buf, "hello");
    expect(912, mystrcmp(buf, "hello"), 0);

    c = 'A';
    d = c + 32;
    expect(913, d, 'a');
    c = 300;
    expect(914, c, 44);
    c = -1;
    expect(915, c, -1);

    gc = 'q';
    if (gc >= 'a') if (gc <= 'z') gc -= 32;
    expect(916, gc, 'Q');
}

/* ---------- scope, preprocessor and sizeof ---------- */

t_scope_prep_size()
{
    int a, x, ia[3];
    char ca[3];

    section("scope/preprocessor/sizeof");
    a = 1;
    x = 0;
    {
        int a;
        a = 2;
        x = a;
    }
    expect(1001, a, 1);
    expect(1002, x, 2);

    expect(1003, TEN, 10);
    expect(1004, TWENTY, 20);
    expect(1005, IFVAL, 11);
    expect(1006, IFNVAL, 13);

    expect(1007, sizeof(char), 1);
    expect(1008, sizeof(int), 2);
    expect(1009, sizeof(unsigned), 2);
    expect(1010, sizeof(ca), 3);
    expect(1011, sizeof(ia), 6);
}

main()
{
    pstr("SBC_SMALLC99 CORE LANGUAGE TEST");
    nl();

    /* BSS must be tested before anything writes those globals. */
    t_bss();
    t_const();
    t_arith();
    t_bits_rel();
    t_logic();
    t_assign();
    t_control();
    t_func();
    t_ptr_char();
    t_scope_prep_size();

    nl();
    pstr("tests: ");
    pnum(tests);
    pstr("  passed: ");
    pnum(passed);
    pstr("  failed: ");
    pnum(failed);
    nl();

    if (failed == 0) pstr("ALL CORE TESTS PASSED");
    else pstr("*** CORE FAILURES ***");
    nl();

    return failed;
}
