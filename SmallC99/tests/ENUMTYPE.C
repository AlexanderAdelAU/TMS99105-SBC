/*
 * ENUMTYPE.C - proposed SMALLC99 enum/typedef regression.
 * NOTE: Native SMALLC99 compilation and SBC execution NOT YET VERIFIED.
 * Expected: 26 passing checks, 0 failures.
 */
extern int putchar();
int tests, passed, failed;
nl() { putchar(13); putchar(10); }
pstr(s) char *s; { while (*s) putchar(*s++); }
pnum(n) int n; {
    if (n < 0) { putchar('-'); n = -n; }
    if (n >= 10) pnum(n / 10);
    putchar('0' + n % 10);
}
expect(id, got, want) int id, got, want; {
    tests++;
    if (got == want) passed++;
    else { failed++; pstr("FAIL "); pnum(id); nl(); }
}
enum direction { NORTH, EAST = 5, SOUTH, WEST = EAST + 4 };
enum { FLAG_A = 1, FLAG_B = 2, FLAG_AB = FLAG_A | FLAG_B };
enum signed_values { NEGATIVE = -3, ZERO = 0, POSITIVE = 3 };

typedef int WORD;
typedef unsigned char BYTE;
typedef WORD *WORDPTR;
typedef struct record RECORD;
typedef RECORD *RECPTR;
typedef int VEC[3];
typedef enum direction DIRECTION;

struct record { WORD value; struct record *next; };

WORD global_word;
BYTE global_byte;
WORDPTR global_ptr;
RECORD global_record;
RECPTR global_recptr;
VEC global_vec;
DIRECTION global_dir;

add(a, b) WORD a, b; { return a + b; }
get_record_value(p) RECPTR p; { return p->value; }

main() {
    WORD w;
    BYTE b;
    WORDPTR p;
    RECORD r;
    RECPTR rp;
    VEC a;
    DIRECTION d;
    enum direction direct;
    int v;
    typedef int LOCALWORD;
    LOCALWORD local;

    expect(1, NORTH, 0);
    expect(2, EAST, 5);
    expect(3, SOUTH, 6);
    expect(4, WEST, 9);
    expect(5, FLAG_A | FLAG_B, 3);
    expect(6, FLAG_AB, 3);
    expect(7, NEGATIVE, -3);
    expect(8, ZERO, 0);
    expect(9, POSITIVE, 3);
    expect(10, WEST - NORTH, 9);

    w = 42; b = 250; p = &w;
    expect(11, w, 42);
    expect(12, b, 250);
    expect(13, *p, 42);
    *p = 17;
    expect(14, w, 17);
    expect(15, add(3, 4), 7);

    r.value = 29; r.next = 0; rp = &r;
    expect(16, get_record_value(rp), 29);
    expect(17, rp->value, 29);
    a[0] = 11; a[1] = 12; a[2] = 13;
    expect(18, a[0], 11);
    expect(19, a[2], 13);

    d = SOUTH; direct = WEST;
    expect(20, d, 6);
    expect(21, direct, 9);
    switch (d) {
    case NORTH: v = 10; break;
    case SOUTH: v = 20; break;
    default: v = -1;
    }
    expect(22, v, 20);

    local = 123;
    expect(23, local, 123);
    global_word = 19; global_byte = 201; global_ptr = &global_word;
    expect(24, *global_ptr, 19);
    expect(25, global_byte, 201);
    global_vec[1] = 77;
    expect(26, global_vec[1], 77);

    pstr("tests: "); pnum(tests);
    pstr(" passed: "); pnum(passed);
    pstr(" failed: "); pnum(failed); nl();
    if (!failed) pstr("ALL ENUM/TYPEDEF TESTS PASSED\r\n");
    return failed;
}
