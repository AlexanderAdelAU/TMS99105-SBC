/*
 * R99 build-size configuration.
 *
 * TEMPORARY SBC TESTING PROFILE.
 *
 * Goal: keep the flat R99.COM image below >C000 when loaded at >0500.
 * These reduced capacities are for initial board bring-up only.
 */

#define LINLEN          128	/*  Must be at least 102: the 4-symbols-per-line table
                                dump at the end of main writes 4 * 25 + 2 bytes into
                                linbuf.  At 96 it spilled 6 bytes into binbuf.  */
#define BUFSIZE          16
#define SYMBOLS         101	/*  MUST BE ODD - slookup's probe step is d = 1, 3, 5, ...
                                and its "table full" test is d == SYMBOLS, which an even
                                SYMBOLS never reaches.  With a power of two the probe
                                only ever visits 4 of the slots (h advances by k*k).  */
#define INCLUDE_DEPTH     1
#define MAXXREF            4
