/* M36b4 literals, arrays, and initialized-data fixture. */

char ch='A';
char bytes[5]={1,2,3};
char text[8]="HELLO";
char *message="OK";
char bytezero[4];

int word=1234;
int words[5]={10,-20,30};
int wordzero[3];
int result;

main()
{
    char *p;
    char *q;

    p="ABC";
    q="XYZ";
    result=ch;
    result+=bytes[2];
    result+=text[1];
    result+=message[0];
    result+=p[2];
    result+=q[1];
    result+=word;
    result+=words[1];
    return result;
}
