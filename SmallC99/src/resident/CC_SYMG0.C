/*
** CC_SYMG0.C -- first half of the 15-character global symbol table.
**
** Global records are fixed 24-byte records: 7 header bytes,
** 15 significant name bytes, NUL, and one pad byte.  The even stride keeps
** every record start aligned; no table is allowed to span an R99 module.
*/
char glbtab0[2400];
