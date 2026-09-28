/* CHAR15.C - 15-significant-character compiler symbol test. */

int globnameAAAAAAA;
int globnameAAAAAAB;
int hashnameQQjnglk;
int hashnamegJhbchu;

funcnameAAAAAAA()
{
        int localnamAAAAAAA;
        int localnamAAAAAAB;

        localnamAAAAAAA = 10;
        localnamAAAAAAB = 1;
        return localnamAAAAAAA + localnamAAAAAAB;
}

funcnameAAAAAAB()
{
        int localnamAAAAAAA;
        int localnamAAAAAAB;

        localnamAAAAAAA = 20;
        localnamAAAAAAB = 2;
        return localnamAAAAAAA + localnamAAAAAAB;
}

main()
{
        globnameAAAAAAA = funcnameAAAAAAA();
        globnameAAAAAAB = funcnameAAAAAAB();
        hashnameQQjnglk = 33;
        hashnamegJhbchu = 44;

        if(globnameAAAAAAA != 11) {
                puts("CHAR15 FAIL1\015");
                return 1;
        }
        if(globnameAAAAAAB != 22) {
                puts("CHAR15 FAIL2\015");
                return 2;
        }
        if(hashnameQQjnglk != 33) {
                puts("CHAR15 FAIL3\015");
                return 3;
        }
        if(hashnamegJhbchu != 44) {
                puts("CHAR15 FAIL4\015");
                return 4;
        }

        puts("CHAR15 PASS\015");
        return 0;
}
