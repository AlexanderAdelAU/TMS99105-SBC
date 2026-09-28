/*
** CC_SYMX.C -- resident continuation of the C symbol arena.
**
** CC_DATA owns and exports symtab[3500] exactly as before. This module is
** linked immediately after CC_DATA and adds 1676 contiguous bytes, giving
** 5176 physical bytes for the 5175-byte 15-character symbol arena.
*/

char symext[1676];
