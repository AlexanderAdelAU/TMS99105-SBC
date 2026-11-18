# XMODEM59 — File-transfer utility

`XMODEM59.A99` implements the SBC's XMODEM transfer utility. The source
includes checksum/CRC receive support, XMODEM-1K packet handling, and an
optional hexadecimal load-address argument.

The root `MAKE_SYSTEM.bat` assembles it to `XMODEM59.H99` and supplies it
to SYSGEN as `XMODEM.COM` in the operating-system volume. All generated
files are staged in `Build/`.
