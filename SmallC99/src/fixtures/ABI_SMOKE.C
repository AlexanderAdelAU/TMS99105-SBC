/* ABI_SMOKE.C - R4-primary/R3-secondary backend smoke test.
 * Keep if/else bodies on separate physical lines for this Small-C parser.
 */
int errors;

check(v)
int v;
{
        if (v == 0)
                errors = errors + 1;
}

main()
{
        int a;
        int b;

        errors = 0;
        a = 7;
        b = 2;

        check(a + b == 9);
        check(a - b == 5);
        check(a * b == 14);
        check(a / b == 3);
        check(a % b == 1);

        check(b < a);
        check(a > b);
        check(b <= a);
        check(a >= b);

        if (errors == 0)
                puts("ABI PASS\015");
        else
                puts("ABI FAIL\015");

        return errors;
}
