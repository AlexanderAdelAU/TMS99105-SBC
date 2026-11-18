;===================================================================
; DECLS.INC - shell 6.5 declarations, shared by the core and every
;             command page.  Hand-maintained.  Generates no code.
;
;   INCL "C:DECLS.INC"     first line of SHELLV65.A99 and of each page
;
; Only things that never move belong here: registers, XOPs, fixed
; memory-map addresses, BDOS function codes, FCB offsets, constants.
; Addresses INSIDE the core (RETURN, PARSENAME, SPACE, CR, ...) move
; whenever the core changes, so MKCINC generates those into CORE.INC.
;
; BDOS is deliberately NOT declared here: the core calls the BDOS at
; >E000 (declared in SHELLV65.A99), while a command page must call the
; BDOS gate BGATE (CORE.INC equates BDOS to it).
;===================================================================

R0	EQU	0
R1	EQU	1
R2	EQU	2
R3	EQU	3
R4	EQU	4
R5	EQU	5
R6	EQU	6
R7	EQU	7
R8	EQU	8
R9	EQU	9
R11	EQU	11
R12	EQU	12
WP	EQU	13
R13	EQU	13
R14	EQU	14
R15	EQU	15
SP	EQU	10		; Hardware Stack Pointer

;---- Define Extended Operations (XOPs) ----
	DXOP	PSEL_DIS, 0	; Disable GAL page selection (use flat memory)
	DXOP	PSEL_EN, 1	; Enable GAL page selection (use mapped memory)
	DXOP	CALL,6
	DXOP	RET,7
	DXOP	PUSH,8
	DXOP	POP,9
	DXOP	WHEX,10		; WRITE OUT A 16 BIT HEX VALUE
	DXOP	RHEX,11		; READ IN A 16 BIT HEX VALUE
	DXOP	WRITE,12	; WRITE CHAR IN MSB
	DXOP	READ,13		; READ CHAR IN MSB
	DXOP	MESG,14		; OUTPUT NULL TERMINATED MESSAGE
	DXOP	DEBUG,15

;==== OPERATING SYSTEM MEMORY LOCATIONS ====
SHELL:	EQU	0C000H		; Shell load address
MONITOR:EQU	0F000H		; ROM Monitor entry point
BANNER:	EQU	MONITOR + 4

MAP_WIN_BASE:	 EQU	080C0H	; 6116 MAP REGISTER CRU BASE
NUM_MAP_ENTRIES: EQU	16	; 16 PAGE REGISTERS

PGLINE	EQU	22		; Number of lines before TYPE pauses output

;==== WORKSPACE AND MEMORY LAYOUT ====
WORKSP:	EQU	0230H		; SHELL WORKSPACE
STACKP:	EQU	0FFEH		; STACK AT >1000, GROWS DOWN
TPA:	EQU	0500H		; DEFAULT LOAD ADDRESS FOR COM FILES
CMDLINE:EQU	0250H		; Command line string buffer
HALT:	EQU	10FFH
COMM_BUF:EQU	0300H		; COMMON BUFFER / LOADERCODE AREA
COMM_FCB:EQU	0280H		; The main File Control Block
FCB:	EQU	COMM_FCB

;==== SYSTEM VARIABLES & POINTERS ====
CMDL_PTR:	EQU	0A0H
CMDL_SIZE:	EQU	0A2H
BUFF_PTR:	EQU	0A4H
FREEMEM:	EQU	0A6H
LBA:		EQU	0A8H
FCB_PTR:	EQU	0ACH
MEMLIMIT:	EQU	0B0H
BUFFSIZ:	EQU	512

SHELL_PTR:	EQU	80H
BDOS_PTR:	EQU	84H
LOADER:		EQU	0300H	; Where LOADERCODE is copied to run
CLSZ:		EQU	64	; Max command line size
BYTSEC:		EQU	512
BYTEWIDE:	EQU	2	; BYTE-PARALLEL CRU TRANSFER WIDTH
NUM_SEGS:	EQU	16	; NUMBER OF MAP SEGMENTS
PROT_SEG:	EQU	12	; FIRST PROTECTED SEGMENT
SEG_SIZE:	EQU	1000H	; 4KB SEGMENT SIZE
INT2_VEC:	EQU	000AH	; INT2 VECTOR ADDRESS
STAGING:	EQU	0500H	; SECTOR STAGING BUFFER

;--- OVERLAY MANAGER PAGE VARIABLES (>02A0->02FF) ---
OVL_PRINT_VEC:	EQU	02A0H
OVL_COUNT:	EQU	02A2H
OVL_PAGE_TAB:	EQU	02A4H
OVL_SEG_TAB:	EQU	02C4H
OVL_CURR_ID:	EQU	02E4H
OVL_CURR_SEG:	EQU	02E6H
FLATBASE:	EQU	1000H	; DEFAULT FLAT LOAD ADDRESS
LGATE:		EQU	02E8H	; COM launch gate (4 bytes)
RGATE:		EQU	02F0H	; COM return gate (6 bytes)
BGATE:		EQU	02F6H	; application BDOS gate (10 bytes)
APAGE:		EQU	1	; private COM physical page
SEGC:		EQU	12	; logical segment C
SEGD:		EQU	13	; logical segment D

IDE_BYTSEC:	EQU	256

;==== BDOS FUNCTION CODES ====
CIN	EQU	1		; Console In
COUT	EQU	2		; Console Out
SELDSK	EQU	14		; Select Disk
RDSEQ	EQU	20		; Read Sequential
FOPEN	EQU	15		; Open File
FCLOSE	EQU	16		; Close File
SEARCH1	EQU	17		; Search First
SEARCH2	EQU	18		; Search Next
ERAFIL	EQU	19		; Erase File
FRENAM	EQU	23		; Rename File
WRSEQ	EQU	21		; Write Sequential
MAKFIL	EQU	22		; Make File
SETDMA	EQU	26		; Set DMA Address
FMKDIR	EQU	41		; Make Directory
FCHDIR	EQU	42		; Change Directory

;==== FCB OFFSETS ====
FCBSIZ	EQU	36		; Total FCB Size
NAM	EQU	0		; Bytes 0-7: Filename
FTY	EQU	11		; Bytes 11: File Extension/Type Code
FLA	EQU	16		; Bytes 16-17: Load Address
CRN	EQU	26		; Bytes 26-27: Current Record Number (Cursor)
CBN	EQU	24		; Bytes 24-25: Current Block Number
RELB	EQU	28		; Bytes 28-29: Relative Block
RELR	EQU	30		; Bytes 30-31: Relative Record
FSB	EQU	12		; Bytes 12-13: File Starting Block
FSZ	EQU	14		; Bytes 14-15: File Size (in sectors)

;==== FILE TYPES ====
SYS	EQU	0
COM	EQU	1
EXE	EQU	2
TXT	EQU	3
BAS	EQU	4
PRO	EQU	5

;==== ASCII CONSTANTS ====
BLKSIZ	EQU	512
NAMSIZ	EQU	11
EXTSIZ	EQU	3
DDEN	EQU	1
DCOMR	EQU	1
PAD	EQU	0
EXTOFF	EQU	NAMSIZ-EXTSIZ	; Offset 8 (Where extension starts in FCB)

