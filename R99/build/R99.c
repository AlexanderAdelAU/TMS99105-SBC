/*
 9900 Cross-Assembler  v. 2.1

 May, 1980

 Copyright (c) 1980 William C. Colley, III.

 File:	r99.c

 It all begins here.
 */

/*  Get globals:  */

#include "stdio.h"
#include "R99CFG.h"
#include "R99gbl.h"
#include "R99Ext.h"
#include "fcntl.h"

extern char srcname[24];
extern avail();

/*  The assembler starts here.  */

/* Diagnostic only: dump the raw Pass-1 symbol table without altering it. */
p1dump()
{
	int i, j;
	struct symbtbl *sp;

	putls("\nPASS 1 SYMBOL TABLE\n");
	putls("SLOT SYMBOL           VALUE FLAGS\n");

	for (i = 0; i < SYMBOLS; i++) {
		sp = symtbl + i;
		if ((sp->symname[0] & 0x7f) != '\0') {
			linptr = linbuf;
			linptr = puthex2(i, linptr);
			*linptr++ = ' ';

			for (j = 0; j < SYMLEN; j++)
				*linptr++ = sp->symname[j] & 0x7f;

			*linptr++ = ' ';
			linptr = puthex4(sp->symvalu, linptr);
			*linptr++ = ' ';
			linptr = puthex2(sp->symflg, linptr);
			*linptr++ = '\n';
			*linptr = '\0';
			putls(linbuf);
		}
	}
}

main(argc, argv)
int argc;
int *argv;
{
	int n, m;
	unsigned u;
	putls("\n----------------------------------------------");
	putls("\nTMS9900 Relocatable Cross-Assembler  vers 2.0\n");
	putls("Copyright (c) 1980  William C. Colley, III\n");
	putls("(TMS 99105A version by Alexander. Cameron Jan 1984 and May 2015 )\n");
	putls("----------------------------------------------\n");
	putls("Available memory = ");
	linptr = linbuf;
	linptr = putdec(avail(), linptr);
	*linptr = '\0';
	putls(linbuf);
	putls(" bytes\n");
	setfiles(argc, argv);

	sympoint = symtbl; /*  Initialize symbol table.	*/
	symend = symtbl + SYMBOLS;
	memset(sympoint, '\0', SYMBOLS * sizeof(struct symbtbl)); /* symflg added  */
	memset(itemflg, '\0', 3); /*  Initialize encode buffer	*/
	memset(symptr, '\0', 3);
	ifsp = 0; /*  Initialise if stack.	*/
	ifstack[ifsp] = 0xffff;
	hxbytes = 0; /*  Initialise hex generator.	*/
	pc = errcount = progsize = 0;
	pass = 1;
	if (lstbuf.fd == CONO)
		puts("Pass 1\n");
	while (pass != 3) /*  The actual assembly starts here.	*/
	{
		errcode = ' ';
		if (!getlin()) {
			strcpy(linbuf, "\tEND\t\t;You forgot this!\n");
			linptr = linbuf;
			markerr('*');
			ifstack[ifsp] = 0xffff;
		}
		asmline(); /*  Get binary from line.	*/
		if (pass > 1) {
			lineout(); /*  In pass 2, list line.	*/
			relout(); /*  In pass 2, build rel file.	*/
		} else
			progsize += nbytes; /* keep track of module size */
		pc += nbytes;
		if (pass == 0) /*  This indicates end of pass 1.	*/
		{
			if (lstbuf.fd == CONO)
				p1dump(); /*  diagnostic: raw slot view of the table  */
			pass = 2;
			pc = 0;
			errcount = 0; /*  pass 2 re-reports every error  */
			ifsp = 0;
			ifstack[ifsp] = 0xffff;
			source_rewind();
			relhead(); /*  module header - must follow the rewind  */
			if (lstbuf.fd == CONO)
				puts("Pass 2\n");
		}
	}
	/*  print statistics  */
	putls("Programme size = ");
linptr = linbuf;
linptr = putdec(progsize, linptr);
*linptr = '\0';
putls(linbuf);
putls("  ");
linptr = linbuf;
linptr = puthex4(progsize, linptr);
*linptr = '\0';
putls(linbuf);
putls("(Hex)\n");
	u = (SYMBOLS - nssymbols);
	u = 100 * u / SYMBOLS;
	putls("\nSymbol table use factor = ");
linptr = linbuf;
linptr = putdec(100 - u, linptr);
*linptr = '\0';
putls(linbuf);
putls("%\n");

	/*  List number of errors.	*/

	linptr = linbuf;
	*linptr++ = '\n';
	if (errcount == 0)
		strcpy(linptr, "No");
	else {
		linptr = putdec(errcount, linptr);
		*linptr = '\0';
	}
	strcat(linbuf, " error(s).\n");
	puts(linbuf);
	if (lstbuf.fd != CONO && lstbuf.fd != NOFILE) {
		putlin(linbuf, &lstbuf);
		/*		putchr('\f',&lstbuf); */
	}
	if (lstbuf.fd != NOFILE) /*  If needed, sort and list
	 symbol table.		*/
	{
		n = nssymbols;
		sympoint = symtbl;
		while (n > 0) {
			linptr = linbuf;
			for (m = 0; m < 4; m++) {
				memcpy(linptr, sympoint->symname, SYMLEN);
				linptr += SYMLEN;					/* Don't know why this works */
				*linptr++ = ' ';
				*linptr++ = ' ';
				linptr = puthex4(sympoint->symvalu, linptr);
				if (sympoint->symflg & EXTBIT)
					*linptr++ = '*'; /* mark external */
				else if (sympoint->symflg & RELBIT)
					*linptr++ = '\''; /* mark relocatable */
				else
					*linptr++ = ' ';

				*linptr++ = ' ';
				*linptr++ = ' ';
				sympoint++;
				if (--n <= 0)
					break;
			}
			*linptr++ = '\n';
			*linptr = '\0';
			putlin(linbuf, &lstbuf);
		}
		putchr(CPMEOF, &lstbuf);
	}
	flush(&lstbuf);
	fclose(sorbuf.fd);
	//wipeout("\r");
}

/*
 Function to set up the file structure.  Routine is called with
 the original argc and argv from main().
 */

setfiles(argc, argv)
int argc;
int *argv;
{
	char sorfname[24], lstfname[24], hexfname[24], *tptr, *arg;
	int noobj;

	/*
	 *  R99 <file> [switches]
	 *
	 *      -L      listing to <file>.L99      (also -LA..-LD, -L-)
	 *      -LX     listing to the console
	 *      -LY     listing to the list device
	 *      -N      no object file - syntax check only
	 *      -R, -H  object to <file>.R99: this is the DEFAULT, and the
	 *              switch is still accepted so old command lines work
	 *      -S      accepted and ignored; the source name is the file name
	 *
	 *  The .R99 object is produced by DEFAULT.  A leading '-' is optional
	 *  on every switch, so the older bare forms (LX, L-, H-) still work.
	 */

	if (--argc == 0)
		wipeout("\nNo file info supplied.\n");
	argv++;

	sorbuf.fd = lstbuf.fd = hexbuf.fd = NOFILE;
	lstbuf.pointr = lstbuf.space;
	hexbuf.pointr = hexbuf.space;
	noobj = 0;

	sorfname[0] = '\0';
	arg = *argv++;
	--argc;
	strcpy(progname, arg); /* copy programme name */
	strcat(sorfname, arg);
	for (tptr = sorfname; *tptr != '\0'; tptr++)
		if (*tptr == '.')
			*tptr = '\0';
	strcpy(lstfname, sorfname);
	strcpy(hexfname, lstfname);
	strcat(sorfname, ".A99");
	strcat(lstfname, ".L99");
	strcat(hexfname, ".R99");

	while (argc > 0) {
		arg = *argv++;
		--argc;
		if (*arg == '-')
			arg++;
		switch (*arg++) {
		case 'L':
			switch (*arg) {
			case 'X':
				lstbuf.fd = CONO;
				break;

			case 'Y':
				lstbuf.fd = LST;
				break;

			default:
				if ((lstbuf.fd = fopen(lstfname, "w")) == 0)
					wipeout("\nCan't open list.\n");
				break;
			}
			break;

		case 'N':
			noobj = 1;
			break;

		case 'H':
		case 'R':
			noobj = 0; /* the object is the default anyway */
			break;

		case 'S':
			break; /* the source name is the file argument */

		default:
			wipeout("\nIllegal command line.\n");
		}
	}

	strcpy(srcname, sorfname);
	if ((sorbuf.fd = fopen(sorfname, "r")) == 0)
		wipeout("\nCan't open source.\n");
	source_rewind();

	/*  Open the object LAST, so a source that will not open cannot
	    leave an empty .R99 behind.  */
	if (noobj == 0)
		if ((hexbuf.fd = fopen(hexfname, "w")) == 0)
			wipeout("\nCan't open R99 file.\n");

}

