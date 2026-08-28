int x;
int y;
int z;
int arr[4];
int *p;

add(a,b)
int a,b;
{
    return a+b;
}

noarg()
{
    return 9;
}

main()
{
    x=8;
    y=2;

    z=add(x,y);
    z=noarg();

    z=x<<y;
    z=x>>y;

    p=arr+x;
    p=x+arr;
    z=p-arr;
}
