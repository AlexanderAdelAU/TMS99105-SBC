/* M36c1 one-level include-file fixture. */

#include "M36INC.H"
#include <M36MORE.H>

int result;

main()
{
    result=incadd(7,8);
    result+=incword;
    result+=INCBASE;
    result+=moreword;
    result+=MOREVAL;
}
