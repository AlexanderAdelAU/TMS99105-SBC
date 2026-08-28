#asm
	AORG 0A000H
#endasm

/*
** CC_CGEN.C -- code-generation back end, M37a r2 complete 8086 overlay
**
** WHY THIS IS AN OVERLAY: the code[] template table plus ccout()
** pushed the RESIDENT image from 0x7C2C to 0x806B, 108 bytes past the
** 0x8000 overlay boundary -- LINK99 correctly refused. code[] is a
** large, PHASE-LOCAL data structure (only codegen touches it), the
** textbook case for a page rather than resident. This is the same
** move OVL_PREP made for the preprocessor.
**
** WHAT MOVED HERE: ccout() (the CCC4 template interpreter),
** setcodes() (fills code[]), and the code[] table itself. The raw
** output primitives stay resident in SPYOUT.A99 and, at M35c, route
** through putc(c,outunit) into TEST.ASM.
**
** WHAT STAYED RESIDENT: gen(), dumpstage() and the whole staging /
** csp machinery -- gen() is called from 45 sites across every
** compiler module and is far too hot to page. gen()'s only tie to
** this page is the final emit, which now goes through the R_CCOUT
** trampoline (and setcodes once at startup through R_SETCODES).
**
** DREL ANCHOR: ccout(). Every module of an overlay must export a name
** DREL recognises or it gets no table row and is never mapped.
*/

#define PCODEMAX 74
#define NAME     7
#define YES      1
#define NO       0

/* M35a/M35b p-code indices -- exact values from Hendrix CC.H. */
#define ADD12     1
#define ADDSP     2
#define AND12     3
#define ANEG1     4
#define ARGCNTn    5
#define ASL12      6
#define ASR12      7
#define CALL1      8
#define CALLm      9
#define BYTE_    10
#define BYTEn    11
#define BYTEr0   12
#define COM1     13
#define DBL1      14
#define DBL2      15
#define DIV12    16
#define DIV12u   17
#define ENTER    18
#define EQ10f    19
#define EQ12     20
#define GE10f    21
#define GE12     22
#define GE12u    23
#define POINT1l  24
#define POINT1m  25
#define GETb1m   26
#define GETb1mu  27
#define GETb1p   28
#define GETb1pu  29
#define GETw1m   30
#define GETw1n   31
#define GETw1p   32
#define GETw2n   33
#define GT10f    34
#define GT12     35
#define GT12u    36
#define WORD_    37
#define WORDn    38
#define WORDr0   39
#define JMPm     40
#define LABm     41
#define LE10f    42
#define LE12     43
#define LE12u    44
#define LNEG1    45
#define LT10f    46
#define LT12     47
#define LT12u    48
#define MOD12    49
#define MOD12u   50
#define MOVE21   51
#define MUL12    52
#define MUL12u   53
#define NE10f    54
#define NE12     55
#define NEARm    56
#define OR12     57
#define POINT1s  58
#define POP2     59
#define PUSH1    60
#define PUTbm1   61
#define PUTbp1   62
#define PUTwm1   63
#define PUTwp1   64
#define rDEC1    65
#define REFm     66
#define RETURN   67
#define rINC1    68
#define SUB12    69
#define SWAP12   70
#define SWAP1s   71
#define SWITCH   72
#define XOR12    73

extern int litlab;
extern pstr();
extern pchar();          /* resident one-character file-output primitive */
extern pdec();
extern int errflag;

/*
** ============================================================
** M35a: REAL CCC4 CODE GENERATION (verbatim 8086 templates)
** ============================================================
**
** AIM (user's strategy): get the compiler working END TO END
** producing the ORIGINAL 8086 output first, using CCC4's own
** code[] templates UNCHANGED. Only once the whole pipeline is
** proven do we retarget the templates to TMS9900. So every
** string below is copied verbatim from CCC4 setcodes() -- do
** NOT rewrite them for TMS9900 yet.
**
** M35a proved the emitter with the minimal local-assignment
** subset. M35b grows code[] in hardware-testable batches, copying
** each template from CCC4 verbatim after the skipped register-flag
** byte. Batch 1 added ADD12, POP2 and PUSH1. Batch 2 adds signed
** arithmetic and logic: SUB12, SWAP12, MUL12, DIV12, MOD12, AND12,
** OR12, XOR12, ANEG1, COM1 and LNEG1. Batch 3 adds the complete
** signed-comparison/control-flow family: EQ/NE/GT/GE/LT/LE as both
** value-producing calls and direct false branches, plus JMPm/LABm.
** Batch 4 adds the complete unsigned arithmetic/comparison family:
** MUL12u, DIV12u, MOD12u, GT12u, GE12u, LT12u and LE12u.
** Batch 5 adds direct and indirect byte/word memory access:
** GETb1m, GETb1mu, GETb1p, GETb1pu, GETw1p, PUTbm1,
** PUTbp1 and PUTwp1. M35d adds direct function calls and
** argument counts, variable shifts, and word-pointer scaling:
** ARGCNTn, CALLm, ASL12, ASR12, DBL1 and DBL2. M36b1 adds
** the remaining core-expression templates: GETw2n plus repeatable
** rINC1/rDEC1 for scalar and pointer pre/post increment/decrement.
** M36b3 adds the switch dispatch/table family: SWITCH, NEARm and
** WORDn. M36b4 adds literal addresses, literal-pool labels, byte/word
** data prefixes and byte/word zero-fill: POINT1l, REFm, BYTE_, BYTEr0,
** WORD_ and WORDr0. M36b5 completes indirect function calls with
** CALL1 and SWAP1s. Every string is copied verbatim from CCC4 after
** the skipped register-flag byte.
**
** THE ENGINE (ccout / template interpreter) is CCC4 outcode()
** ported to our output primitives:
**   - CCC4 emits via fputc(c, output); we emit via pstr/pchar.
**     M35c routes those resident helpers through putc(c,outunit).
**   - pchar() is a resident assembly primitive in SPYOUT.A99.
**     It writes the argument byte directly and uses no C local buffer.
**   - <m> emits the symbol name: baseline outname() writes '_'
**     then the name; our symbols carry text at +NAME (7).
**   - <n> emits a signed decimal via pdec(). <l> emits litlab.
**   - ?..?..? conditional and #..# repeat are copied verbatim.
**
** The first byte of every template is the CCC4 register-flag
** byte (010=AX needed,020=AX zapped,001=BX needed,002=BX
** zapped). The interpreter SKIPS it (cp = code[pcode]+1) exactly
** as baseline does; it matters only to the peephole optimizer.
**
** M37a closes the diagnostic era: every compiler p-code from 1
** through 73 has a real template. A missing table entry records a
** compiler error. The build-time table audit identifies the code.
*/

char *code[PCODEMAX];

setcodes() {
  int i;

  /* Fill the complete compiler p-code table, codes 1 through 73. */
  i = 0;
  while(i < PCODEMAX) code[i++] = 0;

  /* M35b batch 1: templates exposed by the first hardware run. */
  code[ADD12]   = ".ADD AX,BX\n";

  /* M35d: direct calls, variable shifts and pointer scaling. */
  code[ARGCNTn] = ".?MOV CL,<n>?XOR CL,CL?\n";
  code[ASL12]   = ".MOV CX,AX\nMOV AX,BX\nSAL AX,CL\n";
  code[ASR12]   = ".MOV CX,AX\nMOV AX,BX\nSAR AX,CL\n";
  code[CALLm]   = ".CALL <m>\n";
  code[DBL1]    = ".SHL AX,1\n";
  code[DBL2]    = ".SHL BX,1\n";

  /* M36b1: constant secondary-register loads and repeatable steps. */
  code[GETw2n]  = ".?MOV BX,<n>?XOR BX,BX?\n";
  code[rDEC1]   = ".#DEC AX\n#";
  code[rINC1]   = ".#INC AX\n#";

  /* M36b3: switch matcher and inline case table. */
  code[WORDn]   = ". DW <n>\n";
  code[NEARm]   = ". DW _<n>\n";
  code[SWITCH]  = ".CALL __switch\n";

  /* M36b5: indirect function calls and argument/address preservation. */
  code[CALL1]   = ".CALL AX\n";
  code[SWAP1s]  = ".POP BX\nXCHG AX,BX\nPUSH BX\n";

  /* M36b4: literal addresses, literal labels, and data storage. */
  code[BYTE_]   = ". DB ";
  code[BYTEn]   = ". DB <n>\n";
  code[BYTEr0]  = ". DB <n> DUP(0)\n";
  code[POINT1l] = ".MOV AX,OFFSET _<l>+<n>\n";
  code[WORD_]   = ". DW ";
  code[WORDr0]  = ". DW <n> DUP(0)\n";
  code[REFm]    = "._<n>";

  /* M35b batch 2: signed arithmetic and logic templates. */
  code[AND12]   = ".AND AX,BX\n";
  code[ANEG1]   = ".NEG AX\n";
  code[COM1]    = ".NOT AX\n";
  code[DIV12]   = ".CWD\nIDIV BX\n";
  code[LNEG1]   = ".CALL __lneg\n";
  code[MOD12]   = ".CWD\nIDIV BX\nMOV AX,DX\n";
  code[MUL12]   = ".IMUL BX\n";
  code[OR12]    = ".OR AX,BX\n";
  code[SUB12]   = ".SUB AX,BX\n";
  code[SWAP12]  = ".XCHG AX,BX\n";
  code[XOR12]   = ".XOR AX,BX\n";

  /* M35b batch 3: signed comparisons and control flow. */
  code[EQ10f]   = ".OR AX,AX\nJE $+5\nJMP _<n>\n";
  code[EQ12]    = ".CALL __eq\n";
  code[GE10f]   = ".OR AX,AX\nJGE $+5\nJMP _<n>\n";
  code[GE12]    = ".CALL __ge\n";
  code[GT10f]   = ".OR AX,AX\nJG $+5\nJMP _<n>\n";
  code[GT12]    = ".CALL __gt\n";
  code[JMPm]    = ".JMP _<n>\n";
  code[LABm]    = "._<n>:\n";
  code[LE10f]   = ".OR AX,AX\nJLE $+5\nJMP _<n>\n";
  code[LE12]    = ".CALL __le\n";
  code[LT10f]   = ".OR AX,AX\nJL $+5\nJMP _<n>\n";
  code[LT12]    = ".CALL __lt\n";
  code[NE10f]   = ".OR AX,AX\nJNE $+5\nJMP _<n>\n";
  code[NE12]    = ".CALL __ne\n";

  /* M35b batch 4: unsigned arithmetic and comparisons. */
  code[DIV12u]  = ".XOR DX,DX\nDIV BX\n";
  code[GE12u]   = ".CALL __uge\n";
  code[GT12u]   = ".CALL __ugt\n";
  code[LE12u]   = ".CALL __ule\n";
  code[LT12u]   = ".CALL __ult\n";
  code[MOD12u]  = ".XOR DX,DX\nDIV BX\nMOV AX,DX\n";
  code[MUL12u]  = ".MUL BX\n";

  /* M35b batch 5: byte and pointer memory access. */
  code[GETb1m]  = ".MOV AL,<m>\nCBW\n";
  code[GETb1mu] = ".MOV AL,<m>\nXOR AH,AH\n";
  code[GETb1p]  = ".MOV AL,?<n>??[BX]\nCBW\n";
  code[GETb1pu] = ".MOV AL,?<n>??[BX]\nXOR AH,AH\n";
  code[GETw1p]  = ".MOV AX,?<n>??[BX]\n";
  code[PUTbm1]  = ".MOV <m>,AL\n";
  code[PUTbp1]  = ".MOV [BX],AL\n";
  code[PUTwp1]  = ".MOV [BX],AX\n";

  code[ENTER]   = ".PUSH BP\nMOV BP,SP\n";
  code[ADDSP]   = ".?ADD SP,<n>\n??";
  code[POINT1s] = ".LEA AX,<n>[BP]\n";
  code[POINT1m] = ".MOV AX,OFFSET <m>\n";
  code[POP2]    = ".POP BX\n";
  code[PUSH1]   = ".PUSH AX\n";
  code[GETw1n]  = ".?MOV AX,<n>?XOR AX,AX?\n";
  code[GETw1m]  = ".MOV AX,<m>\n";
  code[MOVE21]  = ".MOV BX,AX\n";
  code[PUTwm1]  = ".MOV <m>,AX\n";
  code[RETURN]  = ".?MOV SP,BP\n??POP BP\nRET\n";
  }

/*
** Single-character emission is resident assembly (SPYOUT.A99).
** Do not synthesize a local char[2] here: this smallcp target places
** the second byte at SP+2, which is the saved XOP6 return word.
*/

/*
** ccout(): CCC4 outcode(), ported. Interpret code[pcode]'s
** template, substituting <m>/<n>/<l>, honouring ?..? and #..#.
** A missing template records a compiler error and emits no invalid
** assembler text. The build audit separately proves table completeness.
*/
ccout(pcode, value) int pcode, value; {
  int part, skip, count;
  char *cp, *back;
  char *tp;

  tp = code[pcode];
  if(tp == 0) {
    errflag = 1;
    return;
    }

  part = 0;
  back = 0;
  skip = NO;
  cp = tp + 1;                  /* skip the register-flag byte */

  while(*cp) {
    if(*cp == '<') {
      ++cp;
      if(skip == NO) {
        if(*cp == 'm') { pchar('_'); pstr(value + NAME); }
        else if(*cp == 'n') pdec(value);
        else if(*cp == 'l') pdec(litlab);
        }
      cp += 2;                  /* skip action char and '>' */
      }
    else if(*cp == '?') {
      ++part;
      if(part == 1) { if(value == 0) skip = YES; }
      else if(part == 2) skip = !skip;
      else if(part == 3) { part = 0; skip = NO; }
      ++cp;
      }
    else if(*cp == '#') {
      ++cp;
      if(back == 0) {
        count = value;
        if(count < 1) {
          while(*cp && *cp++ != '#') ;
          }
        else back = cp;
        }
      else {
        --count;
        if(count > 0) cp = back;
        else back = 0;
        }
      }
    else if(skip == NO) pchar(*cp++);
    else ++cp;
    }
  }
