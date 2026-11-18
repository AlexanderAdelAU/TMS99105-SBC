# Building the TMS99105 SBC operating system

This repository contains the source for the current OS system volume: disk
initialiser, BDOS, XMODEM, standalone DIR2 utility, and Shell 6.5 with its
ten command pages. `SYSGEN` packages the assembled modules into a single
Intel HEX payload for the monitor.

## Prerequisites

- Windows with the repository checked out somewhere on drive **C:**.
  The A99 assembler and the existing Shell includes use `C:`-relative paths.
- MinGW `gcc` available on `PATH` to compile `SYSGEN.EXE` from `SYSGEN/sysgen.c`.
- The supplied `a99.exe` and `mkcinc.exe` in the shared `Toolchain` folder.

From the repository root, run:

```bat
MAKE_SYSTEM.bat
```

## Build sequence

| Stage | Source | Output |
| --- | --- | --- |
| 1 | `DSKINIT/SRC/DSKINIT62.A99` | `DSKINIT62.H99` |
| 2 | `BDOS/SRC/BDOS61.A99` | `BDOS61.H99` |
| 3 | `XMODEM/SRC/XMODEM59.A99` | `XMODEM59.H99` |
| 4 | `DIR2/SRC/DIR2.A99` | `DIR2.H99` |
| 5 | `SHELL/SRC/*.A99` and `SHELL/Includes` | Core and ten `.SHC` pages |
| 6 | `SYSGEN/sysgen.c` and all assembled modules | `PAYLOAD65.HEX` |

The first four steps stage sources and the assembler in `Build/Modules`.
Their fresh outputs are collected under `Build/SystemFiles`. The master
batch file passes that folder to `SHELL/MAKESHELL.bat`, which stages its
own inputs in `SHELL/Build` and generates the payload.

On success the final payload is copied to **`MONITOR/PAYLOAD65.HEX`**.
The packer preserves `SHELL.SYS` and `BDOS.SYS` as directory entries 0 and 1,
as required by the bootloader. The build stops if any module fails.

## Building the Shell alone

Run `SHELL/MAKESHELL.bat` without arguments to use the known-working
prebuilt inputs in `SHELL/SystemFiles`. This keeps the previous workflow
available while the complete source build is verified on Windows.

You can also pass a folder containing all four `.H99` dependencies as its
first argument; the master build uses this form automatically.

## Other source files

- `BDOS/Tests/BDTEST62.A99` is an optional BDOS regression test. It is
  deliberately not included in the system payload.
- `SHELL/SRC/DIR2.A99` is an older Shell-internal directory-page variant;
  `DIR2/SRC/DIR2.A99` is the standalone `DIR2.COM` source used by SYSGEN.
- `SHELL/Docs` contains historical Shell design/build notes.
- Older `.H99` files remain in the original `SYSGEN` example folder in
  the repository; the master build deliberately ignores those and uses
  only its newly assembled `Build/SystemFiles` outputs.

Generated `Build` directories, listings, and logs are intentionally ignored
by Git. The checked-in `SHELL/SystemFiles` inputs remain a reproducible
reference until the complete system build is verified on the target machine.
