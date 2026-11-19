/*
** OVLHELLO.C -- overlay GREET, one 4K page at virtual segment 8.
**
** The AORG below is the module's own declaration of where it runs.
** The overlay map names the physical page; this names the segment.
** Every executable overlay runs at >8000, because OVCALL tracks one
** window owner and OVLMGR sets it from segment 8.
**
** String literals are placed in this page by the compiler, so they
** are mapped whenever this code runs.  Passing one to pstr() is safe
** because pstr() is resident: this overlay stays mapped across the
** call.  Passing one to count(), which is in another overlay, would
** NOT be safe - see "THE ONE RULE" in runtime\OVCALL.A99.
*/

#asm
	AORG 8000H
#endasm

hello()
{
    pstr("  hello:    in overlay GREET\n");

    greet();            /* same overlay: the compiler calls it directly,
                        ** no thunk, no mapping, nothing saved         */

    shout();            /* GREET's other page: thunk, but OVCALL finds
                        ** GREET already mapped and branches straight
                        ** through - no mapping, no frame              */

    count(2);           /* overlay BYE: thunk -> OVCALL -> map -> call */

    pstr("  hello:    back from BYE, my page was mapped again\n");
}

greet()
{
    pstr("  greet:    in overlay GREET\n");
}
