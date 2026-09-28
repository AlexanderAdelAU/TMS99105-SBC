#asm
	AORG 8000H
#endasm

	/*
	** CC_STRD.C
	**
	** Struct-declaration overlay (OVL_STRD, ID 8, physical page 15,
	** segment 8).  Parses  struct|union tag [{ member-list }]  and
	** returns the struct's type code.  Reached ONLY through the framed
	** trampoline R_STRUCT (OVLSTUBS), from DECL (globals) and, in the
	** next step, STMT (locals) and DFUN (parameters).  Its own page
	** because Small-C made it ~2.9K: it overflowed DECL's page.
	** Entry point dostruct -> T_STRUCT (generated in OVLADDR.INC).
	*/
	#define BPW       2
	#define IDENT     0
	#define TYPE      1
	#define CLASS     2
	#define SIZE      3
	#define OFFSET    5
	#define LABEL     0
	#define VARIABLE  1
	#define ARRAY     2
	#define POINTER   3
	#define FUNCTION  4
	#define CHR       4
	#define INT       8
	#define UCHR      5
	#define UINT      9
	#define AUTOMATIC 1
	#define STATIC    2
	#define EXTERNAL  3
	#define AUTOEXT   4
	#define DATASEG   1
	#define BYTE_    10
	#define BYTEr0   12
	#define WORD_    37
	#define WORDr0   39
	#define LITMAX   256
	#define NAMESIZE  16

	extern int eof;

	/*
	** ==== Structures (phase 1) ====
	** A struct type code is (tag index << 2) | STRUCTBIT, so it travels
	** through the compiler like CHR and INT; elsize() (CC_STRU, resident)
	** gives its size.  Tags and members live in CC_DATA:
	**   tagtab  NUMTAGS x TAGSIZ   name[16] size[2] first[1] count[1]
	**   memtab  NUMMEMB x SYMMAX   IDENT TYPE CLASS(=tag) SIZE OFFSET NAME
	** Layout (TMS9900): char members at any offset, everything else at
	** an even one; sizes rounded up to even.  A union's members are all
	** at offset 0 and its size is the largest member.
	*/
	extern char tagtab[];
	extern char memtab[];
	extern int ntags;
	extern int nmembs;
	#define STRUCTBIT 2
	#define TAGSIZ   20
	#define TAGSZ    16
	#define TAGFIRST 18
	#define TAGCNT   19
	#define NUMTAGS  16
	#define NUMMEMB  40
	#define SYMMAX   23
	#define NAMEMAX  15
	#define NAME      7

	/*
	** struct|union tag [{ member-list }] -> type code.  Called with the
	** keyword already matched.  The tag is registered BEFORE its body
	** is parsed, so a member may point to the struct being defined.
	** outer: 1 at declaration level; 0 for a member's type, where a
	** body would interleave two structs' members, so it is refused.
	*/
	dostruct(isunion, outer) int isunion, outer; {
	  int tag;
	  char tname[NAMESIZE];
	  if(isunion == 2) return mdrows(outer);   /* [a][b]... after the first */
	  if(symname(tname) == 0) {
	    error("need struct tag");
	    return INT;
	    }
	  if((tag = dfindtag(tname)) == 0) {
	    if(ntags >= NUMTAGS) {
	      error("too many struct tags");
	      return INT;
	      }
	    dnewtag(tname);
	    tag = ntags;
	    }
	  --tag;
	  if(match("{")) {
	    if(outer) dobody(tag, isunion);
	    else error("define struct outside");
	    }
	  return ((tag << 2) | STRUCTBIT);
	  }

	dfindtag(sname) char *sname; {   /* tag index + 1, or 0 */
	  int i;
	  i = 0;
	  while(i < ntags) {
	    if(astreq(sname, tagtab + i * TAGSIZ, NAMEMAX)) return (i + 1);
	    ++i;
	    }
	  return 0;
	  }

	dnewtag(sname) char *sname; {
	  char *t;
	  int k;
	  t = tagtab + ntags * TAGSIZ;
	  k = 0;
	  while(k < TAGSIZ) t[k++] = 0;
	  k = 0;
	  while(an(*sname) && k < NAMEMAX) t[k++] = *sname++;
	  ++ntags;
	  }

	/*
	** { member-list } for tag.  Members are appended to memtab
	** contiguously; tagtab records the first one and the count.
	*/
	dobody(tag, isunion) int tag, isunion; {
	  char *t, *m, mname[NAMESIZE];
	  int mtype, mt, id, dim, msize, off, big;
	  t = tagtab + tag * TAGSIZ;
	  if(t[TAGCNT]) error("struct redefined");
	  t[TAGFIRST] = nmembs;
	  t[TAGCNT] = 0;
	  off = big = 0;
	  while(match("}") == 0) {
	    if(eof) return;
	    if(amatch("char", 4)) mtype = CHR;
	    else if(amatch("unsigned", 8)) {
	      if(amatch("char", 4)) mtype = UCHR;
	      else {
	        amatch("int", 3);
	        mtype = UINT;
	        }
	      }
	    else if(amatch("int", 3)) mtype = INT;
	    else if(amatch("struct", 6)) mtype = dostruct(0, 0);
	    else if(amatch("union", 5)) mtype = dostruct(1, 0);
	    else {
	      error("need member type");
	      kill();
	      continue;
	      }
	    while(1) {
	      if(endst()) break;
	      mt = mtype;               /* per member: char *a, b; */
	      if(match("*")) {
	        id = POINTER;
	        while(match("*")) mt = mt + 64;   /* pointer depth */
	        }
	      else id = VARIABLE;
	      if(symname(mname) == 0) illname();
	      dim = 1;
	      if(match("[")) {
	        if(id == POINTER) mt = mt + 64;   /* array of pointers */
	        id  = ARRAY;
	        dim = needsub();
	        mt  = mdrows(mt);           /* further [..]: rows */
	        }
	      if(id == POINTER) msize = BPW;
	      else {
	        msize = dim * elsize(mt);
	        if(msize == 0) error("incomplete struct");
	        }
	      if((id == POINTER || elsize(mt) != 1) && (off & 1))
	        ++off;                      /* words at even offsets */
	      if(nmembs >= NUMMEMB) {
	        error("too many struct members");
	        return;
	        }
	      m = memtab + nmembs * SYMMAX;
	      m[IDENT] = id;
	      m[TYPE]  = mt;
	      m[CLASS] = tag;
	      putint(msize, m + SIZE, 2);
	      putint(isunion ? 0 : off, m + OFFSET, 2);
	      m = m + NAME;
	      dim = 0;
	      while(an(mname[dim]) && dim < NAMEMAX) *m++ = mname[dim++];
	      *m = 0;
	      ++nmembs;
	      ++t[TAGCNT];
	      if(isunion) {
	        if(msize > big) big = msize;
	        }
	      else off = off + msize;
	      if(match(",") == 0) break;
	      }
	    ns();
	    }
	  if(isunion) off = big;
	  if(off & 1) ++off;
	  putint(off, t + TAGSZ, 2);
	  }

	/*
	** ==== Multi-dimensional arrays ====
	** int m[3][4] is an array of 3 rows, each a row of 4 ints.  A row is
	** a tag-table entry with an empty name (so no struct lookup can find
	** it): size = its byte size, TAGFIRST = its element type.  Its type
	** code is (tag << 2) | 3 - STRUCTBIT and bit 0 together, a combination
	** a struct never uses - so elsize() gives its size unchanged.  Rows are
	** shared: every int[4] row is the same entry.
	**
	** mdrows(elem): called (through R_STRUCT(2, elem)) just after a
	** declarator's first [n]; parses any further [a][b].. and returns the
	** element type for that first dimension - elem itself if there are none.
	*/
	mdrows(elem) int elem; {
	  int d[4], n;
	  n = 0;
	  while(match("[")) {
	    if(n < 4) d[n++] = needsub();
	    else {
	      error("too many dimensions");
	      needsub();
	      }
	    }
	  while(n) elem = mkrow(elem, d[--n]);     /* innermost first */
	  return elem;
	  }

	mkrow(elem, dim) int elem, dim; {
	  char *t;
	  int i, size;
	  elem = elem & 255;
	  if((size = dim * elsize(elem)) == 0) error("need array size");
	  i = 0;
	  while(i < ntags) {                     /* an identical row already? */
	    t = tagtab + i * TAGSIZ;
	    if(t[0] == 0 && (t[TAGFIRST] & 255) == elem
	    && getint(t + TAGSZ, 2) == size) return ((i << 2) | 3);
	    ++i;
	    }
	  if(ntags >= NUMTAGS) {
	    error("too many struct/row types");
	    return elem;
	    }
	  t = tagtab + ntags * TAGSIZ;
	  i = 0;
	  while(i < TAGSIZ) t[i++] = 0;
	  putint(size, t + TAGSZ, 2);
	  t[TAGFIRST] = elem;
	  return ((ntags++ << 2) | 3);
	  }
