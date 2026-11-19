/*
** OVLBYE.C -- overlay BYE, one 4K page at virtual segment 8.
**
** BYE and GREET are mutually exclusive: both run at >8000, so only
** one of them is mapped at a time.  count() below calls back into
** GREET while its own caller, hello(), is still waiting in GREET.
** That is the case a single save word cannot represent and the frame
** stack in OVCALL exists for: two live cross-overlay calls, the
** inner one returning into the overlay the outer one came from.
*/

#asm
	AORG 8000H
#endasm

farewell()
{
    pstr("  farewell: in overlay BYE\n");
}

count(n) int n;
{
    pstr("  count:    in overlay BYE, called from GREET\n");

    if(n > 0) {
        greet();        /* back into GREET, nested inside this call */
        pstr("  count:    back from GREET, my page was mapped again\n");
    }
}
