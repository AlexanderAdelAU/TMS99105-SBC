# DIR2 — Standalone directory utility

`DIR2.A99` builds the standalone `DIR2.COM` utility for BDOS 6.1.
It displays directory entries with filename, type, size in bytes, and
load address, using BDOS directory-search functions.

The root `MAKE_SYSTEM.bat` assembles `DIR2.H99`, then includes it in the
SYSGEN payload as `DIR2.COM`. This source is separate from the Shell's
internal `DIR` command page.
