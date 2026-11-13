#asm
    AORG 9000H
#endasm

/*
** CC_ENUM.C: second page of OVL_STRD, physical page 1 at >9000.
** The enum parser shares OVL_STRD with dostruct, dfindtag and dnewtag
** in CC_STRD.C (>8000). Both pages are mapped simultaneously, so
** calls between them are direct and preserve the ownership contract.
**
** doenum() is the DREL anchor for this page.
*/
#define BPW       2
#define ENUMCONST 5
#define INT       8
#define AUTOMATIC 1
#define STATIC    2
#define NAMESIZE  16
#define NUMTAGS   16
#define TAGSIZ    20
#define TAGSZ     16
#define TAGFIRST  18
#define TAGCNT    19

extern char tagtab[];
extern char *glbptr, *locptr;
extern int ntags, eof, ch;

	/*
	** enum specifier: an enum has the representation of a 16-bit int.
	** Tag names reuse the existing tag namespace (TAGCNT=255 marks
	** enum, TAGFIRST marks a completed definition). Enumerators are
	** ordinary scoped symbols with immediate values in OFFSET.
	**
	** scope: 1 global, 2 local, 0 when used in a member type.
	** R_STRUCT's framed trampoline restores this page after a
	** constexpr() maps the expression overlay.
	*/
	doenum(scope) int scope; {
	  int tag, value, n, made;
	  char tname[NAMESIZE], ename[NAMESIZE];
	  char *t;
	  tname[0] = 0;
	  blanks();
	  if(alpha(ch)) symname(tname);
	  tag = 0;
	  made = 0;
	  if(tname[0]) {
	    tag = dfindtag(tname);
	    if(tag) {
	      t = tagtab + (tag - 1) * TAGSIZ;
	      if((t[TAGCNT] & 255) != 255)
	        error("tag already a struct");
	      }
	    else if(ntags >= NUMTAGS) error("too many enum tags");
	    else {
	      dnewtag(tname);
	      tag = ntags;
	      made = 1;
	      t = tagtab + (tag - 1) * TAGSIZ;
	      t[TAGCNT] = 255;
	      putint(BPW, t + TAGSZ, 2);
	      }
	    }
	  if(match("{")) {
	    if(tag) {
	      if(t[TAGFIRST] && !made) error("enum redefined");
	      t[TAGFIRST] = 1;
	      }
	    value = 0;
	    while(match("}") == 0) {
	      if(eof) return INT;
	      if(symname(ename) == 0) {
	        error("need enumerator name");
	        kill();
	        return INT;
	        }
	      if(match("=")) {
	        n = 0;
	        constexpr(&n);
	        value = n;
	        }
	      if(scope == 2) {
	        if(findloc(ename)) error("enum name redefined");
	        else addsym(ename, ENUMCONST, INT, BPW, value,
	                    &locptr, AUTOMATIC);
	        }
	      else {
	        if(findglb(ename)) error("enum name redefined");
	        else addsym(ename, ENUMCONST, INT, BPW, value,
	                    &glbptr, STATIC);
	        }
	      ++value;
	      if(match(",") == 0) { need("}"); break; }
	      if(match("}")) break;
	      }
	    }
	  else if(tag == 0) error("need enum tag or body");
	  return INT;
	  }

