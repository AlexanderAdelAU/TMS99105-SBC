/* M36b3 switch/case/default completion fixture. */

int selector;
int total;
int result;

main()
{
    total=0;

    selector=2;
    switch(selector) {
    case 0:
        total=100;
        break;
    case 1:
        total=10;
        break;
    case 2:
        total=20;
    case 3:
        total+=3;
        break;
    default:
        total=999;
    }

    selector=7;
    switch(selector) {
    case 4:
        total+=40;
        break;
    default:
        total+=7;
    }

    selector=9;
    switch(selector) {
    case 8:
    case 9:
        total+=90;
        break;
    default:
        total=0;
    }

    selector=-1;
    switch(selector) {
    case -1:
        total+=1;
        break;
    default:
        total=0;
    }

    selector=99;
    switch(selector) {
    case 1:
        total=1;
        break;
    }

    selector=1;
    switch(selector) {
    case 1:
        switch(total) {
        case 121:
            total+=200;
            break;
        default:
            total=9999;
        }
        break;
    default:
        total=8888;
    }

    result=total;
    return result;
}
