; OVLDEFS.INC ? fixed architecture definitions

OVL_EXPR    EQU 1
OVL_STMT    EQU 2
OVL_DECL    EQU 3
OVL_PREP    EQU 4		; M33: recycles the retired OVL_TESTB slot.
;			  Codegen has been RESIDENT since Step 6, so ID 4
;			  was an empty table row. It now carries the CCC2
;			  preprocessor (CC_PREP.C) and its macro pools.
OVL_DFUN 	EQU 5
OVL_CGEN    EQU 6		; M35a: codegen back end (ccout + code[] table)
OVL_CLI     EQU 7		; M36a r1: one-shot command-line parser overlay

VEC_R_EXPR  EQU 02A8H
VEC_R_STMT  EQU 02A2H
VEC_R_DECL  EQU 02A4H
VEC_R_TEST  EQU 02A6H
VEC_R_CEXPR EQU 02AAH
VEC_R_STR   EQU 02ACH
VEC_R_PREP  EQU 02AEH
VEC_R_DEF   EQU 02B0H
VEC_R_CCOUT EQU 02B2H
VEC_R_SETCODES EQU 02B4H
