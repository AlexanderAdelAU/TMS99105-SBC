/* M36b1 core-expression completion fixture. */

int x;
int y;
int z;
int arr[4];
int *p;

main()
{
    x=5;
    y=3;
    p=arr;

    z=x+7;
    z=arr[2];

    z=++x;
    z=x++;
    z=--x;
    z=x--;

    ++p;
    --p;

    x+=3;
    x-=2;
    x*=4;
    x/=2;
    x%=3;
    x&=7;
    x|=8;
    x^=1;
    x<<=1;
    x>>=1;

    z=x?y:7;
    z=x&&y;
    z=x||y;
    z=sizeof(int);
    z=(x=1,y=2,x+y);
}
