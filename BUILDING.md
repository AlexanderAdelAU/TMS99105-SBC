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
3. Assemble `DSKINIT62.A99`, `BDOS61.A99`, `XMODEM59.A99` and the
   standalone `DIR2.A99` from `SRC/SHELL`.
4. Assemble `SHELLV65.A99` and generate `Build/CORE.INC` from its listing
   using MKCINC.
5. Assemble all ten Shell command-page `.A99` files.
6. Assemble `BDTEST62`, `IDE_DEBUG`, `MAPDEBUG`, and `MEMTEST` from
   `SRC/Utilities` into standalone `.H99` files; these are **not** passed to SYSGEN.
7. Pack the fourteen OS files into `Build/PAYLOAD65.HEX` using SYSGEN.
8. After a successful build, copy the four utility `.H99` files and then
   the completed payload to `MONITOR/` for upload to the SBC.

**Important:** The packer must keep `SHELL.SYS` and `BDOS.SYS` as the first
two directory entries because the bootloader locates them by position.

The script deletes each previous assembler output before rebuilding it,
checks A99's success message, and stops if any component fails. No payload
is published unless the entire build succeeds.

`SRC/Utilities` contains the standalone uploadable test/debug programs.
`BDTEST62.A99` and `IDE_DEBUG.A99` are copied from their original component
folders to keep these utilities together without removing the original files.
The utility outputs remain outside the fourteen-file operating-system payload.

`BDTEST62.H99` is intended for execution by the Shell at `>1000`; the debug
and memory-test utilities use the Disc Monitor entry address `>0500`.
See [the Utilities README](SRC/Utilities/README.md) before running them.

## Disc Monitor and upload staging

`SRC/DISC_MONITOR` holds the independent ROM monitor source and its existing
`IDE_DEBUG.A99` reference copy. The monitor ROM is **not** part of SYSGEN
and is not assembled by `MAKE_SYSTEM.bat`; the utility copy of `IDE_DEBUG`
under `SRC/Utilities` **is** assembled as a standalone uploadable program.

The separate `MONITOR/` directory is the upload-staging location for files
to be transferred to the SBC. It is not the monitor's source directory.

`Build` and `MONITOR` outputs are generated locally and excluded from Git.
The complete build requires Windows and should be verified there.
