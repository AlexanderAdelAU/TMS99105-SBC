# Building the TMS99105 SBC system volume

## Requirements

- Windows, with the repository located on **drive C:**. A99 and the
  assembly includes rely on `C:`-relative filenames.
- MinGW `gcc` available on `PATH`.
- The assembler executables in `Toolchain`.

From the repository root, run `MAKE_SYSTEM.bat`.

## Build sequence

1. Copy the tools, maintained include files, and all required source files
   from their `SRC/<component>` folders into the `Build` directory.
2. Compile `SRC/SYSGEN/sysgen.c` into `Build/SYSGEN.EXE`.
3. Assemble `DSKINIT62.A99`, `BDOS61.A99`, `XMODEM59.A99` and `DIR2.A99`.
4. Assemble `SHELLV65.A99` and generate `Build/CORE.INC` from its listing
   using MKCINC.
5. Assemble all ten Shell command-page `.A99` files.
6. Pack the fourteen files into `Build/PAYLOAD65.HEX` using SYSGEN.
7. Copy the completed payload to `MONITOR/PAYLOAD65.HEX`.

**Important:** The packer must keep `SHELL.SYS` and `BDOS.SYS` as the first
two directory entries because the bootloader locates them by position.

The script deletes each previous assembler output before rebuilding it,
checks A99's success message, and stops if any component fails. No payload
is published unless the entire build succeeds.

`SRC/BDOS/BDTEST62.A99` is supplied as a regression-test program; it is
not part of the fourteen-file operating-system payload.

`Build` and `MONITOR` outputs are generated locally and excluded from Git.
The source build must be verified on Windows before merging the refactor
into `main`.
