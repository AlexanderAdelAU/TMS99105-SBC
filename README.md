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

The hardware drawings and historical project material remain available
in the repository's earlier `main` history while this refactor is tested.
