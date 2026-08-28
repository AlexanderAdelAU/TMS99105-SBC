/* M36b2 statement-control completion fixture. */

int i;
int j;
int total;
int result;

main()
{
    i=0;
    total=0;

    while(i<6) {
        i++;
        if(i==2)
            continue;
        if(i==5)
            break;
        total+=i;
    }

    j=0;
    do {
        j++;
        total+=j;
    } while(j<3);

    for(i=0;i<3;i++) {
        for(j=0;j<4;j++) {
            if(j==1)
                continue;
            if(i==2)
                break;
            total+=i+j;
        }
    }

    i=0;
    for(;;) {
        i++;
        if(i==2)
            continue;
        total+=i;
        if(i==3)
            break;
    }

    goto done;
    total=999;

done:
    result=total;
    return result;
}
