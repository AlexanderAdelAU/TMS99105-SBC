/* M36b5 indirect-function-call fixture. */

int result;
int *gfp;

add(a,b)
int a,b;
{
    return a+b;
}

sub(a,b)
int a,b;
{
    return a-b;
}

noarg()
{
    return 40;
}

main()
{
    int (*lfp)();

    gfp=add;
    result=gfp(7,5);

    lfp=sub;
    result+=lfp(9,4);

    lfp=noarg;
    result+=lfp();

    return result;
}
