;*******************************************************************
; TMSREGS.INC -- SmallC99 TMS99000 backend register contract
;
; Generated TMS99000 code uses SP as the software-stack register name.
; Do not emit R10 in target templates; use SP.
;*******************************************************************

R0      EQU     0               ; backend/helper scratch
R1      EQU     1               ; backend/helper scratch, MPY/DIV pair
R2      EQU     2               ; backend/helper scratch
R3      EQU     3               ; primary accumulator and return value
R4      EQU     4               ; secondary operand, normally clobbered
R5      EQU     5               ; argument count / shift helper input
R6      EQU     6               ; reserved, preserved by runtime helpers
R7      EQU     7               ; reserved, preserved by runtime helpers
R8      EQU     8               ; reserved, preserved by runtime helpers
FP      EQU     9               ; frame pointer
SP      EQU     10              ; software stack pointer (requested alias)
R11     EQU     11              ; BL link register
R12     EQU     12              ; CRU base / platform reserved
WP      EQU     13              ; cached current workspace pointer
R14     EQU     14              ; context reserved
R15     EQU     15              ; context reserved
