#asm
	AORG 9000H
#endasm

/*
** CC_MACS.C -- macro pools, page 2 of OVL_PREP, milestone 33a
**
** DREL ANCHOR: putmac(). Every module of a multi-page overlay must
** export a name DREL recognises, or it gets no OVL_TABLE row, is never
** mapped, and the first reference into it lands in unmapped memory.
** A pure-data module cannot be anchored, so putmac() lives here rather
** than in CC_PREP.C -- it is the one function that touches nothing but
** these pools, which makes it the honest choice as well as the
** convenient one.
**
** WHY THIS MODULE EXISTS
**
** CC_PREP.C assembles to about 2418 bytes and pools sized for
** self-hosting are another 3600. Together they overran a single 4KB
** page by 482 bytes (DREL spec 2.5). Rather than shave MACNBR down to
** where self-hosting would fail later -- the mistake STAGESZ 128 made
** and had to be undone at M32 -- OVL_PREP became a TWO-PAGE overlay,
** the same mechanism the CCC3 expression engine already uses for its
** four pages.
**
** Both pages are mapped SIMULTANEOUSLY whenever OVL_PREP is mapped, so
** preprocess() and dodefine() in the >8000 page reference macn, macq
** and putmac here at >9000 with ordinary direct code. No trampoline,
** no remap, nothing to think about at the call site.
**
** DO NOT give this module a second AORG, and do not let it grow past
** its page: the pools are sized so 200 macros plus putmac() fit with
** roughly 400 bytes spare, and the build prints the number.
*/

#define NAMESIZE  9

/*
** MUST MATCH THE SAME BLOCK IN CC_PREP.C. Duplicated deliberately --
** every page of the expression engine duplicates its p-code defines
** the same way, because these modules share an overlay, not a source
** file.
**
**     macn   MACNBR * (NAMESIZE+2)   name + NUL + 2-byte macq index
**     macq   MACNBR * 7              replacement text, NUL-separated
*/
#define MACNBR   200
#define MACNSIZE (MACNBR*(NAMESIZE+2))
#define MACQSIZE (MACNBR*7)
#define MACMAX   (MACQSIZE-1)

extern int macptr;

char macn[MACNSIZE];
char macq[MACQSIZE];

/*
** Store one character of a macro's replacement text. CCC1 putmac().
**
** Baseline clamps at MACMAX and lets dodefine() report the overflow
** afterwards; kept verbatim, including returning the character so
** dodefine's "while(putmac(gch()));" terminates on the NUL.
*/
putmac(c) char c;
{
    macq[macptr] = c;
    if(macptr < MACMAX) ++macptr;
    return c;
}
