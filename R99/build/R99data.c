/*
 TMS990 Series Cross-Assembler

 File: R99DATA.c

 Single owner of R99 shared assembler storage.

 R99gbl.h contains the declarations used by the normal assembler modules.
 This module deliberately does not include R99gbl.h because Small-C/Plus
 rejects a real definition after an extern declaration of the same object.

 BUFSIZE, LINLEN and SYMBOLS come directly from R99CFG.h.
 SYMLEN and IFDEPTH match the declarations in R99gbl.h.
 */

#include "R99CFG.h"

#define SYMLEN  16
#define IFDEPTH 16
#define FILE char

struct diskbuf {
	int fd;
	unsigned char *pointr;
	unsigned char *endpoint;
	unsigned char space[BUFSIZE];
	FILE *fp;
};

struct symbtbl {
	char symname[SYMLEN];
	unsigned symvalu;
	unsigned char symflg;
	unsigned char sympad;	/*  keeps the entry EVEN in length - see below  */
};

/*  sympad is not optional.  Without it an entry is 19 bytes, so every odd
    numbered slot starts on an ODD address and symvalu (offset 16) lands on
    an odd address too.  The 99105 forces word accesses even, so the store
    goes to offset 15/16 instead of 16/17 and wipes symname[15].  The value
    reads back consistently, but the name no longer matches, so symcmp fails
    and the same symbol gets added twice.  */

/* Shared assembler storage. */

struct diskbuf sorbuf, lstbuf, hexbuf;

struct symbtbl symtbl[SYMBOLS], *symend, *sympoint;

unsigned ifsp;
unsigned ifstack[IFDEPTH + 1];

char *linptr, linbuf[LINLEN];

unsigned char binbuf[LINLEN], *bptr;
int nbytes;

char chksum, hxbytes, *hxlnptr, hxlnbuf[88];

unsigned char extflg;
unsigned char entflg;
unsigned char relflg;
int errcode;
int errcount;
int evalerr;
unsigned backflg, oldattr;
unsigned oldvalu;
int curdrive;
int hexflg;
int directok;
unsigned pc;
unsigned progsize;
unsigned address;
char pass;
char addrmode;
char quitflag;
unsigned char quoteflg;

unsigned char outchunk,
outrem,
item,
type;
unsigned field;

char symbol[SYMLEN],
progname[SYMLEN];
unsigned nssymbols;
unsigned itemflg[3];
unsigned symptr[3];
