/* M36c1 quoted include fixture. */

#define INCBASE 40

int incword=2;

incadd(a,b)
int a,b;
{
    return a+b+INCBASE;
}
