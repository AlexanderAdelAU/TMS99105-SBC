/*
** OVLGRET2.C -- the SECOND page of overlay GREET, at virtual segment 9.
**
** GREET is declared in the overlay map as two modules, so OVLMGR maps
** both of its pages together and they are co-resident whenever GREET
** owns the window.  Two consequences, and they are the reason a
** multi-page overlay exists:
**
**   1. shout() below calls greet(), which is in GREET's other page.
**      That is an ordinary direct call: no thunk, no mapping, nothing
**      saved.  An overlay may be as large as the window.
**
**   2. hello(), in GREET's first page, calls shout() through its
**      generated thunk, because the two are in different modules and
**      the compiler cannot know they share an overlay.  OVCALL sees
**      that GREET already owns the window and branches straight
**      through - the fast path.  The map-register trace shows no
**      write for that call.
**
** The AORG is what places this page at segment 9.  Segment 8 belongs
** to the overlay's first module, which is what makes GREET the window
** owner; LINK99 fails the link if that is ever not the case.
*/

#asm
	AORG 9000H
#endasm

shout()
{
    pstr("  shout:    GREET page 2, reached by thunk with no mapping\n");

    greet();            /* GREET page 1: direct call across the pages
                        ** of one overlay, no trampoline               */
}
