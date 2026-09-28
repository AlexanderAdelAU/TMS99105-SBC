/* SYM15.C - verifies 15-significant-character identifiers. */

#define LONGMACRO_ONE 10
#define LONGMACRO_TWO 20

int globalsymbolone;
int globalsymboltwo;

longfunctionone()
{
        int localvarnameone;
        int localvarnametwo;

        localvarnameone = LONGMACRO_ONE;
        localvarnametwo = 1;
        return localvarnameone + localvarnametwo;
}

longfunctiontwo()
{
        int localvarnameone;
        int localvarnametwo;

        localvarnameone = LONGMACRO_TWO;
        localvarnametwo = 2;
        return localvarnameone + localvarnametwo;
}

main()
{
        globalsymbolone = longfunctionone();
        globalsymboltwo = longfunctiontwo();

        if (globalsymbolone != 11) {
                puts("SYM15 FAIL1\015");
                return 1;
        }
        if (globalsymboltwo != 22) {
                puts("SYM15 FAIL2\015");
                return 2;
        }

        puts("SYM15 PASS\015");
        return 0;
}
