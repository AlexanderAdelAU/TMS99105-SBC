/*
 * R99SYMB.c
 *
 * Symbol-table routines for the TMS9900/99105 relocatable assembler.
 * Kept in K&R declaration style to match the rest of this source tree.
 */

#include "R99CFG.h"
#include "R99gbl.h"
#include "R99Ext.h"

/*
 * Look up SYMBOL in the hashed symbol table.
 *
 * Returns:
 *   1  symbol not found and table full
 *   0  symbol found; sympoint points at it
 *  -1  symbol not found; sympoint points at the free slot
 */
slookup(symbol)
char *symbol;
{
    int h;
    int d;

    d = 1;
    h = hash(symbol);

    for (;;) {
        sympoint = symtbl + h;

        if ((sympoint->symname[0] & 0x7f) == '\0')
            return -1;

        if (symcmp(symbol, sympoint->symname) == 0)
            return 0;

        h += d;
        d += 2;

        if (h >= SYMBOLS)
            h -= SYMBOLS;

        if (d == SYMBOLS)
            return 1;
    }
}

/* Return the hash-table index for a fixed-width symbol name. */
hash(symbol)
char *symbol;
{
    int i;
    unsigned j;
    unsigned tsymbr;
    unsigned tsymbl;

    j = 0;
    for (i = 0; i < (SYMLEN / 2); i++) {
        tsymbl = (*symbol++ & 0x7f) << 7;
        tsymbr = (*symbol++ & 0x7f) << 1;
        j += tsymbl + tsymbr;
    }

    /*  smallcp has no unsigned divide: % compiles to the SIGNED _ccdiv,
        so a 16-bit j with bit 15 set returns a NEGATIVE index and slookup
        then walks memory below symtbl.  j passes 32767 for almost every
        symbol (eight blank pairs alone come to 33280), so clear the sign
        bit before the modulo.  */
    j &= 0x7fff;

    return j % SYMBOLS;
}

/*
 * Count the symbol table entries.  If FLAG is zero (SORT), compact the
 * occupied entries at the start of symtbl and sort them by symbol name.
 */
sortsym(flag)
unsigned char flag;
{
    int n;
    int i;
    int t;
    struct symbtbl *tptr;

    n = 0;
    i = 0;
    t = sizeof(struct symbtbl);

    sympoint = symtbl;

    /*
     * Do NOT write this as:
     *
     *     for (tptr = symtbl; tptr < symend; tptr++)
     *
     * Small-C/Plus 1.06a miscompiles post-increment of a LOCAL struct
     * pointer when the pointed-to object is larger than one byte.  For
     * this 20-byte structure it stores (tptr + 20) through tptr instead
     * of storing it back into the local tptr variable.  The loop then
     * never advances and corrupts the symbol table until the machine
     * faults.
     *
     * Recompute the pointer from an integer slot index instead.
     */
    while (i < SYMBOLS) {
        tptr = symtbl + i;

        if ((tptr->symname[0] & 0x7f) != '\0') {
            if (!flag) {
                if (sympoint != tptr)
                    memcpy(sympoint, tptr, t);
                sympoint = sympoint + 1;
            }
            n = n + 1;
        }

        i = i + 1;
    }

    return n;
}

/* Compare the fixed-width symbol names, ignoring the high bit. */
symcmp(sym1, sym2)
char *sym1;
char *sym2;
{
    int i;
    int t;

    t = 0;
    for (i = 0; i < SYMLEN; i++) {
        t = ((*sym1++ & 0x7f) - (*sym2++ & 0x7f));
        if (t != 0)
            break;
    }

    return t;
}

/* Add SYMBOL if it is not already in the table. */
addsym(symbol)
char *symbol;
{
    int t;

    if (lstbuf.fd == CONO)	/*  diagnostic: -LX only  */
        puts3("ADDSYM: [", symbol, "]\n");

    t = slookup(symbol);
    if (t > 0)
        wipeout("\nSymbol Table Overflow.\n");

    if (t != 0) {
        memcpy(sympoint->symname, symbol, SYMLEN);
        sympoint->symvalu = 0;
        sympoint->symflg = 0;

        if (entflg)
            sympoint->symflg |= ENTBIT;
        if (extflg)
            sympoint->symflg |= EXTBIT;

        /* All symbols begin relocatable; pseudo-ops/instructions may clear it. */
        sympoint->symflg |= RELBIT;
    }

    return t;
}

/* Abort an assembly with an explanatory message. */
wipeout(reason)
char *reason;
{
    puts(reason);
    exit(-1);
}
