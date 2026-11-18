# TMS99105 SBC — Shell 6.5 System Build

This repository contains the current source build for the TMS99105 SBC
operating-system volume: DSKINIT62, BDOS61, XMODEM59, standalone DIR2,
Shell 6.5 and its ten command pages.

**One command builds the complete operating-system volume:**

```bat
MAKE_SYSTEM.bat
```

The script stages every component in `Build`, assembles all modules,
generates the Shell address include, runs SYSGEN and publishes the resulting
`PAYLOAD65.HEX` to `MONITOR`.

## Repository layout

| Folder | Purpose |
| --- | --- |
| `SRC/BDOS` | BDOS 6.1 and its regression-test source |
| `SRC/DISC_MONITOR` | Independent Disc Monitor ROM source; not part of SYSGEN |
| `SRC/DSKINIT` | Disk initialiser 6.2 |
| `SRC/XMODEM` | XMODEM 5.9 |
| `SRC/DIR2` | Standalone directory utility |
| `SRC/SHELL` | Shell 6.5 core and ten command pages |
| `SRC/SYSGEN` | Host-side system volume packer (`sysgen.c`) |
| `Includes` | Maintained assembler include files |
| `Toolchain` | Windows A99 assembler and MKCINC utility |
| `Build` | Generated working files; created automatically |
| `MONITOR` | Upload-staging directory for files transferred to the SBC; created automatically |

`SRC/DISC_MONITOR` contains the Disc Monitor source. It is built separately
from the operating-system payload and is **not** included by SYSGEN.

See [BUILDING.md](BUILDING.md) for prerequisites and build details.

The hardware drawings and historical project material remain available
in the repository's earlier `main` history while this refactor is tested.
