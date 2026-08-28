/*
** CC_SCAN_SYM.C
**
** Resident scanner and symbol-table services.
** Scanner functions are the already-proven framework versions.
** Symbol-table functions are restored from the baseline Small-C source.
*/

#define YES 1
#define NULL 0

#define IDENT  0
#define TYPE   1
#define CLASS  2
#define SIZE   3
#define OFFSET 5
#define NAME   7

#define SYMAVG  12
#define SYMMAX  16
#define NUMLOCS 25
#define NUMGLBS 200
#define NAMEMAX 8
#define NAMESIZE 9

#define STARTLOC symtab
#define ENDLOC   (symtab + NUMLOCS * SYMAVG)
#define STARTGLB ENDLOC
#define ENDGLB   (STARTGLB + NUMGLBS * SYMMAX)

char *line, *lptr;
int ch, nch, eof, errflag, nxtlab;

/*
** ==== M33: preprocessor line buffers ====
**
** mline holds the RAW source line, pline the PREPROCESSED one.
** preprocess() (CC_PREP, OVL_PREP) writes both; the scanner walks
** whichever "line" points at and must see them from every page, so
** these two buffers are the entire resident cost of the preprocessor
** -- its code and its macro pools live out in the overlay.
**
** They sit here rather than in CC_DATA because line/lptr/ch/nch are
** here: this is the scanner's storage, and these are scanner buffers.
**
** msname is the macro name scratch, shared by preprocess() and
** dodefine() (both overlay) with search() (resident), so it is
** resident for the same reason ssname is.
**
** blanks() compares "line == mline" to decide whether refilling is
** possible, so mline must be a distinct object, not an alias.
*/
#define LINESIZE 128

char mline[LINESIZE];
char pline[LINESIZE];
char msname[NAMESIZE];

extern int opindex;
extern int opsize;

extern char symtab[];
extern char *glbptr;
extern char *locptr;
extern char *cptr;
extern char *cptr2;
extern char *cptr3;

avail(x) int x; {
  }

isalpha(c) char c; {
  return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
  }

isdigit(c) char c; {
  return (c >= '0' && c <= '9');
  }

gch() {
  int c;
  if(c = ch) bump(1);
  return c;
  }

bump(n) int n; {
  if(n) lptr += n;
  else  lptr  = line;
  if(ch = nch = *lptr) nch = *(lptr+1);
  }

white() {
  avail(YES);
  return (*lptr <= ' ' && *lptr);
  }

alpha(c) char c; {
  return (isalpha(c) || c == '_');
  }

an(c) char c; {
  return (alpha(c) || isdigit(c));
  }

/*
** ==== M33: the preprocess() stub is gone ====
**
** It set eof and returned, which is why every fixture up to milestone
** 32 was a single line. The real CCC2 preprocess() now lives in
** CC_PREP.C (OVL_PREP) and blanks() reaches it through R_PREP below.
*/

blanks() {
  while(1) {
    while(ch) {
      if(white()) gch();
      else return;
      }
    if(line == mline) return;
    R_PREP();
    if(eof) break;
    }
  }

streq(str1, str2) char str1[], str2[]; {
  int k;
  k = 0;
  while(str2[k]) {
    if(str1[k] != str2[k]) return 0;
    ++k;
    }
  return k;
  }

match(lit) char *lit; {
  int k;
  blanks();
  if(k = streq(lptr, lit)) {
    bump(k);
    return 1;
    }
  return 0;
  }

astreq(str1, str2, len) char str1[], str2[]; int len; {
  int k;
  k = 0;
  while(k < len) {
    if(str1[k] != str2[k]) break;
    if(str2[k] < ' ') break;
    if(str1[k] < ' ') break;
    ++k;
    }
  if(an(str1[k]) || an(str2[k])) return 0;
  return k;
  }

amatch(lit, len) char *lit; int len; {
  int k;
  blanks();
  if(k = astreq(lptr, lit, len)) {
    bump(k);
    return 1;
    }
  return 0;
  }

/*
** ==== STEP 4a ====
**
** Test the source against a space-separated list of operators and
** report which one matched. Baseline CCC2 nextop(), verbatim.
**
**     opindex  index of the matched operator within the list
**     opsize   its length in characters
**
** The caller adds a level-specific offset to opindex and uses the
** result to index op[]/op2[] (CC_DATA), which is how a level turns a
** matched operator into a p-code. down() then does bump(opsize) to
** step the scanner past it.
**
** Two guards, both load-bearing:
**
**   *(lptr+opsize) != '='   so "<" does not match the "<" of "<=",
**                           and "=" does not match "=="
**   != *(lptr+opsize-1)     so "<" does not match "<<", "&" does not
**                           match "&&", "|" not "||"
**
** NOTE the local "char op[4]" deliberately shadows the global
** "int op[16]" for the duration of this function -- it is a scratch
** buffer for the candidate operator text, nothing to do with the
** p-code table. This is baseline's own naming; kept so the function
** stays a verbatim match with CCC2.
*/
nextop(list) char *list; {
  char op[4];
  int k;
  opindex = 0;
  blanks();
  while(1) {
    opsize = 0;
    while(*list > ' ') op[opsize++] = *list++;
    op[opsize] = 0;
    if(opsize = streq(lptr, op))
      if(*(lptr+opsize) != '=' &&
         *(lptr+opsize) != *(lptr+opsize-1))
         return 1;
    if(*list) {
      ++list;
      ++opindex;
      }
    else return 0;
    }
  }

symname(sname) char *sname; {
  int k;
  blanks();
  if(alpha(ch) == 0) return (*sname = 0);
  k = 0;
  while(an(ch)) {
    sname[k] = gch();
    if(k < NAMEMAX) ++k;
    }
  sname[k] = 0;
  return 1;
  }

error(msg) char *msg; {
  errflag = 1;
  }

/*
** ==== M33: reaches preprocess() through R_PREP, like blanks() ====
**
** This was the SECOND caller of preprocess() in this file and it was
** missed when the preprocessor became an overlay. A direct call from
** resident code to a window-A address jumps into whatever page happens
** to be mapped -- with OVL_EXPR mapped that is the middle of
** CC_EXPR_A, and execution runs off into the symbol table.
**
** The chain lint shows this instantly: a resident module must never
** carry an XREFS group for an overlay-only entry point. The build now
** checks for exactly that.
*/
inbyte() {
  while(ch == 0) {
    if(eof) return 0;
    R_PREP();
    }
  return gch();
  }

need(str) char *str; {
  if(match(str) == 0) error("missing token");
  }

ns() {
  if(match(";") == 0) error("no semicolon");
  else errflag = 0;
  }

skip() {
  if(an(inbyte()))
       while(an(ch)) gch();
  else while(an(ch) == 0) {
    if(ch == 0) break;
    gch();
    }
  blanks();
  }

endst() {
  blanks();
  return (streq(lptr, ";") || ch == 0);
  }

getlabel() {
  return(++nxtlab);
  }

/*********** symbol table management functions ***********/

addsym(sname, id, type, size, value, lgpp, class)
  char *sname, id, type;
  int size, value, *lgpp, class; {

  if(lgpp == &glbptr) {
    if(cptr2 = findglb(sname)) return cptr2;
    if(cptr == 0) {
      error("global symbol table overflow");
      return 0;
      }
    }
  else {
    if(locptr > (ENDLOC - SYMMAX)) {
      error("local symbol table overflow");
      return 0;
      }
    cptr = *lgpp;
    }

  cptr[IDENT] = id;
  cptr[TYPE]  = type;
  cptr[CLASS] = class;
  putint(size,  cptr + SIZE,   2);
  putint(value, cptr + OFFSET, 2);

  cptr3 = cptr2 = cptr + NAME;
  while(an(*sname)) *cptr2++ = *sname++;

  if(lgpp == &locptr) {
    *cptr2 = cptr2 - cptr3;
    *lgpp = ++cptr2;
    }

  return cptr;
  }

/*
** Search a fixed-record table.
** On failure cptr identifies the empty slot, or is zero if full.
*/
search(sname, buf, len, end, max, off)
  char *sname, *buf, *end;
  int len, max, off; {

  cptr = cptr2 = buf + ((hash(sname) % (max - 1)) * len);

  while(*cptr != NULL) {
    if(astreq(sname, cptr + off, NAMEMAX)) return 1;
    if((cptr = cptr + len) >= end) cptr = buf;
    if(cptr == cptr2) return (cptr = 0);
    }

  return 0;
  }

hash(sname) char *sname; {
  int i, c;
  i = 0;
  while(c = *sname++) i = (i << 1) + c;
  return i;
  }

findglb(sname) char *sname; {
  if(search(sname, STARTGLB, SYMMAX, ENDGLB, NUMGLBS, NAME))
    return cptr;
  return 0;
  }

findloc(sname) char *sname; {
  cptr = locptr - 1;

  while(cptr > STARTLOC) {
    cptr = cptr - *cptr;
    if(astreq(sname, cptr, NAMEMAX)) return (cptr - NAME);
    cptr = cptr - NAME - 1;
    }

  return 0;
  }

/*
** ==== M30: step to the next local symbol record ====
**
** CCC2.C nextsym(), verbatim. compound() (CC_STMT) walks the local
** table with this to copy LABEL entries down over dead variable
** entries when a block closes -- labels are function-scoped.
**
** Local records are terminated by the LENGTH BYTE that addsym
** writes ("*cptr2 = cptr2 - cptr3"), always <= NAMEMAX and so
** below ' '. That is the terminator scanned for here.
*/
nextsym(entry) char *entry; {
  entry = entry + NAME;
  while(*entry++ >= ' ');    /* find length byte */
  return entry;
  }

/*
** Integers in compiler tables are stored low byte first.
*/
getint(addr, len) char *addr; int len; {
  int i;
  i = *(addr + --len);
  while(len--) i = (i << 8) | (*(addr + len) & 255);
  return i;
  }

putint(i, addr, len) char *addr; int i, len; {
  while(len--) {
    *addr++ = i;
    i = i >> 8;
    }
  }
