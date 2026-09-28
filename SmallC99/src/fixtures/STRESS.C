/* STRESS.C
 * Pointer comparison stress test for SmallC99 / TMS99105.
 *
 * Expected output:
 *
 *     POINTER STRESS: SUCCESS
 *
 * The test executes all four unsigned pointer relations thousands of
 * times, including values across the >7FFF/>8000 boundary.
 */

int arena[33];
int errors;
int checks;
int result;

ceq(got,want)
int got;
int want;
{
        checks = checks + 1;
        if (got != want)
                errors = errors + 1;
}

/* One static occurrence of each pointer relation. */
pv(a,b,lt,le,gt,ge)
int *a;
int *b;
int lt;
int le;
int gt;
int ge;
{
        ceq(a < b,lt);
        ceq(a <= b,le);
        ceq(a > b,gt);
        ceq(a >= b,ge);
}

matrix()
{
        int *p;
        int *q;
        int i;
        int j;

        p = arena;
        i = 0;

        while (i <= 32) {
                q = arena;
                j = 0;

                while (j <= 32) {
                        if (i < j)
                                pv(p,q,1,1,0,0);
                        else {
                                if (i == j)
                                        pv(p,q,0,1,0,1);
                                else
                                        pv(p,q,0,0,1,1);
                        }

                        q = q + 1;
                        j = j + 1;
                }

                p = p + 1;
                i = i + 1;
        }
}

raw_boundary()
{
        int *lo;
        int *hi;
        int *small;
        int *top;

        /*
         * Never dereferenced.  These values deliberately cross bit 15
         * so signed pointer comparison would fail.
         */
        lo = 0x7ffe;
        hi = 0x8000;
        small = 0x0002;
        top = 0xfffe;

        pv(lo,hi,1,1,0,0);
        pv(hi,lo,0,0,1,1);

        pv(small,top,1,1,0,0);
        pv(top,small,0,0,1,1);

        pv(hi,hi,0,1,0,1);
}

loop_forms()
{
        int *b;
        int *e;
        int *last;
        int *p;
        int n;

        b = arena;
        e = arena + 32;
        last = e - 1;

        p = b;
        n = 0;
        while (p < e) {
                p = p + 1;
                n = n + 1;
        }
        ceq(n,32);

        p = e;
        n = 0;
        while (p > b) {
                p = p - 1;
                n = n + 1;
        }
        ceq(n,32);

        p = b;
        n = 0;
        while (p <= last) {
                p = p + 1;
                n = n + 1;
        }
        ceq(n,32);

        p = last;
        n = 0;
        while (p >= b) {
                n = n + 1;
                if (p == b)
                        break;
                p = p - 1;
        }
        ceq(n,32);
}

array_operand()
{
        int *e;

        e = arena + 32;
        pv(arena,e,1,1,0,0);
}

main()
{
        errors = 0;
        checks = 0;
        result = -1;

        matrix();
        raw_boundary();
        loop_forms();
        array_operand();

        /*
         * Do not use ceq() for the final check-count test because ceq()
         * itself increments checks.
         */
        if (checks != 4384)
                errors = errors + 1;

        result = errors;

        if (errors == 0) {
                puts("POINTER STRESS: SUCCESS");
                return 0;
        }

        puts("POINTER STRESS: FAILURE");
        return errors;
}
