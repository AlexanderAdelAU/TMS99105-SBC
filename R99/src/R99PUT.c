/*
 TMS9900/99105  Cross-Assembler  v. 1.0

 January, 1985

 Original 6800 version Copyright (c) 1980 William C. Colley, III.
 Modified for the TMS9900/99105  and to be relocatable by Alexander Cameron.

 File:	a99put.c

 List and hex output routines.
 */

/*  Get globals:  */

#include "R99CFG.h"
#include "R99gbl.h"
#include "R99Ext.h"

/*
 Function to form the list output line and put it to
 the list device.  Routine also puts the line to the
 console in the event of an error.
 */
lineout() {
	char tbuf[25], *tptr, *bptr, conbuf[LINLEN];
	int count, test;
	memset(tbuf, ' ', 24);
	tbuf[24] = '\0';
	memset(conbuf, '\0', LINLEN);
	tptr = tbuf;
	*tptr++ = errcode;
	tptr++;
	if (hexflg != NOCODE)
		tptr = puthex4(address, tptr);
	else
		tptr += 4;
	tptr += 3;

	/*
	 * BSS uses hexflg == FLUSH and nbytes as the amount by which the
	 * relocatable location counter advances.  Those bytes do not exist
	 * in binbuf, so listing them would read beyond the encoding buffer.
	 */
	if (hexflg == FLUSH) {
		putlin(tbuf, &lstbuf);
		putlin(linbuf, &lstbuf);

		if (lstbuf.fd != CONO && errcode != ' ') {
			strcat(conbuf, tbuf);
			strcat(conbuf, linbuf);
			puts(conbuf);
		}
		return;
	}

	count = 0;
	bptr = binbuf;
	while (TRUE) {
		test = nbytes;
		if ((count == nbytes) || (count != 0) && (count % 4 == 0)) {
			putlin(tbuf, &lstbuf);
			if (count > 4)
				putchr('\n', &lstbuf);
			else
				putlin(linbuf, &lstbuf);
			if (lstbuf.fd != CONO && errcode != ' ') {
				strcat(conbuf, tbuf);
				if (count >= 5)
					putchar('\n');
				else {
					linbuf[28] = '\n';
					linbuf[29] = '\0';
					strcat(conbuf, linbuf);
				}
				puts(conbuf);
			}
			tptr = tbuf + 2;
			tptr = puthex4(address, tptr);
			memset(tptr, ' ', 14);
			tptr += 3;
		}
		if (count == nbytes)
			return;
		count++;
		address++;
		tptr = puthex2(*bptr++, tptr);
		if (count % 2 == 0)
			tptr++;
	}
}

/*
 Function to put a 4-digit hex number into an output line.
 */

puthex4(number, lineptr)
unsigned number;
char *lineptr;
{
	lineptr = puthex2(number >> 8, lineptr);
	return puthex2(number, lineptr);
}

/*
 Function to put a 2-digit hex number into an output line.
 */

puthex2(number, lineptr)
unsigned char number;
char *lineptr;
{
	unsigned n;

	/*  Small-C passes a 16-bit word and the unsigned char declaration does
	    NOT truncate it, so mask here.  The value of the assignment below is
	    the whole word, not the byte stored: for the low half of 0x1000,
	    (0x1000 >> 4) + '0' = 0x130, which is > '9', so the '0' just stored
	    got 7 added to it and printed as '7' - 0x1000 came out as "1070".
	    Masking also keeps the >> away from the arithmetic shift's sign bit.  */
	n = number & 0x00ff;

	if ((*lineptr = (n >> 4) + '0') > '9')
		*lineptr += 7;
	lineptr++;
	if ((*lineptr = (n & 0x0f) + '0') > '9')
		*lineptr += 7;
	lineptr++;
	return lineptr;
}

/*
 Function to put a decimal number into an output line.
 */

putdec(number, lineptr)
unsigned number;
char *lineptr;
{
	if (number == 0)
		return lineptr;
	lineptr = putdec(number / 10, lineptr);
	*lineptr++ = number % 10 + '0';
	return lineptr;
}

/*
 Function to move a line to a disk buffer.  The line is pointed to
 by line, and the disk buffer is specified by its disk I/O buffer
 structure dskbuf.
 */

putlin(line, dskbuf)
char *line;
struct diskbuf *dskbuf;
{
	while (*line != '\0')
		putchr(*line++, dskbuf);
}

/*
 Function to put a character into a disk buffer.  The character
 is sent in char, and the disk buffer is specified by the address
 of its structure.  Newline characters (LF's) are converted to
 CR/LF pairs.
 */

putchr(byte, dskbuf)
char byte;
struct diskbuf *dskbuf;
{
	/* byte &= 0x7f; */
	switch (dskbuf->fd) {
	case CONO:
		if (byte != CPMEOF)
			putchar(byte);

	case NOFILE:
		return;

	case LST:
		if (byte != CPMEOF) {
			/*	if (byte == '\n') bdos(LISTOUT,'\r');
			 bdos(LISTOUT,byte);
			 */
		} else
			putchr('\n', dskbuf);
		return;

	default:
		if (dskbuf->fd >= 20) {
			putls("In putchr: invalid fd\n");
			return;
		}
		putbyt(byte, dskbuf);
		/*  smallcp compiles '\n' as CR alone, so a listing file held
		    no line feeds at all and TYPE ran the whole assembly onto
		    one line.  Follow every CR with an LF on the way to disk.
		    The console (CONO) and the REL file (rputchr) do not come
		    through here, so neither is affected.  */
		if (byte == '\n')
			putbyt(LFCHAR, dskbuf);
		return;

	}
}

/*
 Put one raw byte into a disk buffer, writing the buffer out when it
 fills.  Split out of putchr so that a CR can be followed by an LF.
 */

putbyt(byte, dskbuf)
char byte;
struct diskbuf *dskbuf;
{
	*(dskbuf->pointr)++ = byte;
	if (dskbuf->pointr >= dskbuf->space + BUFSIZE) {
		if (write(dskbuf->fd, dskbuf->space, BUFSIZE) == -1) {
			putls("\nDisk write error ++a99_0001\n");
			wipeout("\n");
		}
		dskbuf->pointr = dskbuf->space;
	}
}
rputchr(byte, dskbuf)
unsigned char byte;
struct diskbuf *dskbuf;
{
	/*  This tested dskbuf->fp, a FILE * that this port never sets, so the
	    guard never fired.  With no H option hexbuf.fd is NOFILE (-1) and
	    pass 2 would buffer REL bytes and then call write(-1, ...).  Test
	    the unit number instead, as putchr does.  */
	if (dskbuf->fd < LODISK || dskbuf->fd >= 20)
		return;
	*(dskbuf->pointr)++ = byte;
	if (dskbuf->pointr >= dskbuf->space + BUFSIZE) {
		if (write(dskbuf->fd, dskbuf->space, BUFSIZE) == -1) {
			//	if (fwrite(dskbuf->space,sizeof(char),BUFSIZE,dskbuf->fp) == -1) {
			putls("\nDisk write error ++a99_0002\n");
			wipeout("\n");
		}
		dskbuf->pointr = dskbuf->space;
	}
}

/*
 Function to flush a disk buffer.
 */

flush(dskbuf)
struct diskbuf *dskbuf;
{
	unsigned t;
	if (dskbuf->fd < LODISK)
		return;
	t = dskbuf->pointr - dskbuf->space;
	t = (t % BUFSIZE == 0) ? t / BUFSIZE + 1 : t;
	while (dskbuf->pointr < dskbuf->space + BUFSIZE)
		*(dskbuf->pointr)++ = 0;
	if (write(dskbuf->fd, dskbuf->space, t) == -1)
		wipeout("\nDisk write error in hexflush.\n");
	if (fclose(dskbuf->fd) == 0)
		wipeout("\nError closing file.\n");
}

/*
 Function to flush a disk buffer.
 */

rflush(dskbuf)
struct diskbuf *dskbuf;
{
	unsigned t;
	if (dskbuf->fd < LODISK)
		return;
	t = dskbuf->pointr - dskbuf->space;
	t = (t % BUFSIZE == 0) ? t / BUFSIZE + 1 : t;
	while (dskbuf->pointr < dskbuf->space + BUFSIZE)
		*(dskbuf->pointr)++ = 0;
//	if ((t=fwrite(dskbuf->space,sz,t,dskbuf->fp)) <= 0 ) wipeout("\nDisk write error in rflush.\n");
	if (write(dskbuf->fd, dskbuf->space, t) == -1)
		wipeout("\nDisk write error in rflush.\n");
	if (fclose(dskbuf->fd) == 0)
		wipeout("\nError closing file.\n");
}

