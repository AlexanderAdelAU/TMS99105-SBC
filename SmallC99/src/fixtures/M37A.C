extern int extword;
extern int extfun();

int data = 3;
char text[] = "OK";

main()
{
    return extfun(extword) + data + text[0] + latecall(5);
}
