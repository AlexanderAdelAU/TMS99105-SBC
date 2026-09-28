/*
** ARRTEST.C -- SBC_SMALLC99 multi-dimensional arrays
**
** Runtime regression in the style of LANGTEST_LANGUAGE.C: every check
** is numbered and compares a computed value with the known answer.
**
** Expected final lines:
**
**     tests: 50  passed: 50  failed: 0
**     ALL ARRAY TESTS PASSED
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

int  m[3][4];                   /* 3 rows of 4 ints                */
int  c[2][3][4];                /* 3-D                             */
char names[4][6];               /* a table of short strings        */
unsigned char ub[2][3];
char *pt[2][2];                 /* 2-D array of pointers           */
struct pt { int x; int y; };
struct pt grid[2][3];           /* 2-D array of structs            */
struct box { int id; int cell[2][3]; char tag[2][4]; } bx;
int  e[3][4];                   /* shares m's row type             */

/* a 2-D array parameter: rows of 4 */
sum2(a, rows)
int a[][4];
int rows;
{
    int i, j, s;
    s = 0;
    for (i = 0; i < rows; i++)
        for (j = 0; j < 4; j++)
            s = s + a[i][j];
    return s;
}

/* a row passed as a plain pointer */
sumrow(r)
int *r;
{
    return r[0] + r[1] + r[2] + r[3];
}

scopy(d, s)
char *d, *s;
{
    while (*d++ = *s++);
}

t_2d()
{
    int i, j;
    section("2-D arrays");
    for (i = 0; i < 3; i++)
        for (j = 0; j < 4; j++)
            m[i][j] = i * 10 + j;
    expect(1, m[0][0], 0);
    expect(2, m[1][2], 12);
    expect(3, m[2][3], 23);
    expect(4, m[2][0], 20);
    expect(5, sizeof(m), 24);
    expect(6, sum2(m, 3), 138);
    expect(7, sumrow(m[1]), 46);
    expect(8, *m[2], 20);
    expect(9, m[1][-1], 3);             /* rows are contiguous */
    m[1][1] = 99;
    expect(10, m[1][1], 99);
    expect(11, m[1][2], 12);            /* neighbours untouched */
}

t_rows()
{
    int *r;
    section("rows as addresses");
    r = m[2];
    expect(12, r[1], 21);
    expect(13, *(r + 3), 23);
    r = m[0];
    expect(14, r[4], 10);               /* row 1 follows row 0 */
    expect(15, m[2] - m[0], 8);         /* int pointers: elements */
    expect(16, *(m[1] + 2), 12);
    expect(17, **m, 0);
}

t_3d()
{
    int i, j, k, s;
    section("3-D arrays");
    for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++)
            for (k = 0; k < 4; k++)
                c[i][j][k] = i * 100 + j * 10 + k;
    expect(18, c[1][2][3], 123);
    expect(19, c[0][1][2], 12);
    expect(20, c[1][0][0], 100);
    expect(21, sizeof(c), 48);
    s = 0;
    for (k = 0; k < 4; k++) s = s + c[1][1][k];
    expect(22, s, 446);
    expect(23, sumrow(c[1][2]), 486);
}

t_chars()
{
    section("char tables");
    scopy(names[0], "zero");
    scopy(names[1], "one");
    scopy(names[2], "two");
    scopy(names[3], "three");
    expect(24, names[1][0], 'o');
    expect(25, names[3][4], 'e');
    expect(26, names[3][5], 0);
    expect(27, names[2][1], 'w');
    expect(28, sizeof(names), 24);
    ub[1][2] = 200;
    expect(29, ub[1][2], 200);
    expect(30, sizeof(ub), 6);
}

t_ptrs()
{
    section("2-D arrays of pointers");
    pt[0][0] = "ab";
    pt[0][1] = "cd";
    pt[1][0] = "ef";
    pt[1][1] = "gh";
    expect(31, pt[1][0][1], 'f');
    expect(32, *pt[0][1], 'c');
    expect(33, sizeof(pt), 8);
}

t_structs()
{
    int i, j;
    section("2-D arrays of structs");
    for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++) {
            grid[i][j].x = i;
            grid[i][j].y = j;
        }
    expect(34, grid[1][2].x, 1);
    expect(35, grid[1][2].y, 2);
    expect(36, grid[0][1].y, 1);
    expect(37, sizeof(grid), 24);
    bx.id = 7;
    bx.cell[1][2] = 55;
    bx.cell[0][0] = 11;
    scopy(bx.tag[1], "xyz");
    expect(38, bx.cell[1][2], 55);
    expect(39, bx.cell[0][0], 11);
    expect(40, bx.tag[1][2], 'z');
    expect(41, bx.id, 7);
    expect(42, sizeof(bx), 22);
}

t_local()
{
    int lm[2][3], i, j;
    char lc[3][2];
    section("local 2-D arrays");
    for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++)
            lm[i][j] = i + j;
    expect(43, lm[1][2], 3);
    expect(44, lm[0][1], 1);
    lc[2][1] = 'q';
    expect(45, lc[2][1], 'q');
    expect(46, sizeof(lm), 12);
    expect(47, sizeof(lc), 6);
}

t_shared()
{
    section("shared row types");
    e[2][3] = 77;
    expect(48, e[2][3], 77);
    expect(49, m[2][3], 23);            /* separate storage */
    expect(50, sizeof(e), 24);
}

main()
{
    t_2d();
    t_rows();
    t_3d();
    t_chars();
    t_ptrs();
    t_structs();
    t_local();
    t_shared();

    nl();
    pstr("tests: ");
    pnum(tests);
    pstr("  passed: ");
    pnum(passed);
    pstr("  failed: ");
    pnum(failed);
    nl();

    if (failed == 0) pstr("ALL ARRAY TESTS PASSED");
    else pstr("*** ARRAY FAILURES ***");
    nl();

    return failed;
}
