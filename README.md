# TMS99105 SBC — System Build

This repository contains the current source build for the TMS99105 SBC
operating-system volume: DSKINIT62, BDOS61, XMODEM59, standalone DIR2,
Shell 6.5 and its ten command pages.

**One command builds the complete operating-system volume:**

```bat
MAKE_SYSTEM.bat
```

The script stages every component in `Build`, assembles all modules,
generates the Shell address include, runs SYSGEN and publishes the resulting
`PAYLOAD65.HEX` to the sibling `../MONITOR` directory. It also builds the standalone utilities
from `SRC/Utilities` and copies their `.H99` files to `../MONITOR` for individual
upload; they are not included in the SYSGEN volume.

## Repository layout

| Folder | Purpose |
| --- | --- |
| [`SRC/BDOS`](SRC/BDOS/README.md) | BDOS 6.1 and its regression-test source |
| [`SRC/DISC_MONITOR`](SRC/DISC_MONITOR/README.md) | Independent Disc Monitor ROM source; not part of SYSGEN |
| [`SRC/DSKINIT`](SRC/DSKINIT/README.md) | Disk initialiser 6.2 |
| [`SRC/Utilities`](SRC/Utilities/README.md) | Standalone uploadable test and debug programs; excluded from SYSGEN |
| [`SRC/XMODEM`](SRC/XMODEM/README.md) | XMODEM 5.9 |
| [`SRC/SHELL`](SRC/SHELL/README.md) | Shell 6.5 core, ten command pages, and standalone DIR2 |
| [`SRC/SYSGEN`](SRC/SYSGEN/README.md) | Host-side system volume packer (`sysgen.c`) |
| `Includes` | Maintained assembler include files |
| `Toolchain` | Windows A99 assembler and MKCINC utility |
| `Build` | Generated working files; created automatically |
| `../MONITOR` | Sibling upload-staging directory for `PAYLOAD65.HEX` and utility `.H99` files; created automatically |

`SRC/DISC_MONITOR` contains the Disc Monitor source. It is built separately
from the operating-system payload and is **not** included by SYSGEN.

Each source component has its own README beside its source files.
See [BUILDING.md](BUILDING.md) for the complete build instructions.

## Hardware drawings and project references

The original hardware and reference material has been restored alongside
this build, with its original directory names:

- [`Documents`](Documents/) — processor manuals and reference documents.
- [`Schematics`](Schematics/) — SBC schematic PDFs, including versions 2–5.
- [`Memory Structure`](Memory%20Structure/) — architecture and memory-map drawings.
- [`Memory Mapper`](Memory%20Mapper/) — how the memory mapper hardware works, with links to the overlay system that uses it.
- [`ovl99_demo`](ovl99_demo/) — OVLDEMO, a minimal OVL99 overlay application: sources, map, runtime, tools and a one-step Windows build.
- [`images`](images/) — hardware photographs and illustrations.

The original root-level schematic drawings are now together under
[`Schematics`](Schematics/Readme.md), including the
[`SBC schematic (PDF)`](Schematics/SBC_Schematic.pdf),
[`SBC schematic (PNG)`](Schematics/SBC_Schematic.png),
[`Memory Mapping Schematic`](Schematics/Memory%20Mapping%20Schematic.png), and
[`Memory Mapped Architecture Schematic`](Schematics/Memory%20Mapped%20Architecture%20Schematic.png).

These reference materials are not inputs to `MAKE_SYSTEM.bat`. The old
`Disc_Monitor` source folder remains consolidated under `SRC/DISC_MONITOR`,
and the previously separated SmallC99 project is not duplicated here.
