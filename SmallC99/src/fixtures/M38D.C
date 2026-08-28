/* M38d byte movement fixture. */

char gch='A';
char bytes[4]={1,2,3,4};
char *bp;
int result;

echo(c)
char c;
{
    return c;
}

main()
{
    char local;

    bp=bytes;
    local=gch;
    gch='Z';
    bp[1]=local;
    result=gch;
    result+=bp[1];
    result+=echo(bp[2]);
    return result;
}
