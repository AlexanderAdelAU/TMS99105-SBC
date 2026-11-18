# DSKINIT62 — Disk initialiser

`DSKINIT62.A99` is the source for the disk initialisation/bootstrap
component used by the TMS99105 SBC system volume. It begins at address
`>0500`, prepares the IDE disk structures, and writes the initial system
files supplied by the SYSGEN payload.

The root `MAKE_SYSTEM.bat` assembles this source to `DSKINIT62.H99` before
running SYSGEN. Unlike the disk files (`SHELL.SYS`, `BDOS.SYS`, etc.),
DSKINIT is passed to SYSGEN as the boot initialiser rather than a named
directory entry.

The maintained source is here; generated `.H99`, listing and log files
are kept in the root `Build/` directory.
