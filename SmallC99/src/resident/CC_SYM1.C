/*
** CC_SYM1.C -- continuation of the resident symbol arena.
** 3000 + 2176 = 5176; the compiler uses 5175 bytes. The spare byte
** keeps the complete arena even-sized without changing ENDGLB.
*/

char symtail[2176];
