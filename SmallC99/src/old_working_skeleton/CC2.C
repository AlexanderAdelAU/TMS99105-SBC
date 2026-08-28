/*
** CC2.C - first real smallcp compile test.
** gch(), bump(), white(), alpha(), an() -- verbatim from the
** original Small-C source. All resident, no overlay crossing,
** same functions already hand-ported/verified in CC2.A99 -- so we
** have known-correct behavior to check the compiled output against.
**
** Fixes from the first link attempt (confirmed unresolved symbols):
**  - line/lptr/ch/nch: real globals now, not extern (nothing else
**    defines them since CC2.A99 was retired).
**  - YES: #define instead of relying on cc.h, so it's a compile-time
**    constant, not a link-time symbol.
**  - avail: stubbed as a no-op. Genuinely unported still; this is
**    the honest placeholder, not a silent drop.
**  - isalpha/isdigit: real implementations, not fakes -- the actual
**    logic is trivial (range checks), no reason to stub what's this
**    easy to just do correctly.
**  _cceq is NOT touched here -- it's a compiler-generated runtime
**  helper (equality-comparison support), so it needs clib99.LIB;
**  faking that one would be guessing at codegen internals we don't
**  actually know.
*/

#define YES 1
#define NAMEMAX 8

char *line, *lptr, *mline;
int ch, nch, eof, errflag, nxtlab;

avail(x) int x; {
  /* stub: real stack/symbol-table overflow guard not ported yet */
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

alpha(c)  char c; {
  return (isalpha(c) || c == '_');
  }

an(c)  char c; {
  return (alpha(c) || isdigit(c));
  }

/*
** -------- restored: CC1.C's real functions (declglb, dofunction,
** decl, etc.) call these directly -- not stubs, genuine external
** calls. Verbatim from the real uploaded source, previously proven
** correct on hardware, re-added here since CC1.C depends on them. ---
*/

/*
** preprocess() -- stub. Real version reads a new physical line
** (macro expansion, quoting, comment stripping) via ifline()/
** inline(). None of that exists yet -- setting eof is the honest
** substitute so blanks()'s real control flow still terminates
** correctly instead of hanging.
*/
preprocess() {
  eof = 1;
  }

/*
** blanks() -- verbatim. Loops skipping whitespace; at end-of-line,
** returns immediately if this is a macro-expansion line (line==
** mline -- never true yet), otherwise calls preprocess().
*/
blanks() {
  while(1) {
    while(ch) {
      if(white()) gch();
      else return;
      }
    if(line == mline) return;
    preprocess();
    if(eof) break;
    }
  }

/*
** streq(str1,str2) -- verbatim. Returns the length of str2 if it
** matches the start of str1, 0 otherwise. NOT a boolean.
*/
streq(str1, str2)  char str1[], str2[]; {
  int k;
  k = 0;
  while (str2[k]) {
    if(str1[k] != str2[k]) return 0;
    ++k;
    }
  return k;
  }

/*
** match(lit) -- verbatim.
*/
match(lit)  char *lit; {
  int k;
  blanks();
  if(k = streq(lptr, lit)) {
    bump(k);
    return 1;
    }
  return 0;
  }

/*
** astreq(str1,str2,len) -- verbatim.
*/
astreq(str1, str2, len)  char str1[], str2[]; int len; {
  int k;
  k = 0;
  while (k < len) {
    if(str1[k] != str2[k]) break;
    if(str2[k] < ' ') break;
    if(str1[k] < ' ') break;
    ++k;
    }
  if(an(str1[k]) || an(str2[k])) return 0;
  return k;
  }

/*
** amatch(lit,len) -- verbatim.
*/
amatch(lit, len)  char *lit; int len; {
  int k;
  blanks();
  if(k = astreq(lptr, lit, len)) {
    bump(k);
    return 1;
    }
  return 0;
  }

/*
** symname(sname) -- verbatim, including real failure behavior
** (writes NUL into sname[0]) and real NAMEMAX truncation.
*/
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

/*
** error(msg) -- stub. Real version writes to stderr/listing with
** source-line context -- no I/O binding exists yet. Sets errflag
** (the one piece of state other functions check), nothing else.
*/
error(msg) char *msg; {
  errflag = 1;
  }

/*
** inbyte() -- verbatim.
*/
inbyte()  {
  while(ch == 0) {
    if(eof) return 0;
    preprocess();
    }
  return gch();
  }

/*
** need(str)/ns() -- verbatim.
*/
need(str)  char *str; {
  if(match(str) == 0) error("missing token");
  }

ns()  {
  if(match(";") == 0) error("no semicolon");
  else errflag = 0;
  }

/*
** skip() -- verbatim.
*/
skip() {
  if(an(inbyte()))
       while(an(ch)) gch();
  else while(an(ch) == 0) {
    if(ch == 0) break;
    gch();
    }
  blanks();
  }

/*
** endst() -- verbatim.
*/
endst() {
  blanks();
  return (streq(lptr, ";") || ch == 0);
  }

/*
** getlabel() -- verbatim.
*/
getlabel() {
  return(++nxtlab);
  }
