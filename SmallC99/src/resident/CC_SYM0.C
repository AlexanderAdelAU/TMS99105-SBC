/*
** CC_SYM0.C -- first resident chunk of the 15-character symbol arena.
** Must link immediately before CC_SYM1. 3000 is even and below 4KB.
*/

char symtab[3000];
