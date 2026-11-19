# Utilities — standalone programs for the TMS99105 SBC

These sources are assembled by the root `MAKE_SYSTEM.bat`, but **none is
included in `PAYLOAD65.HEX` or SYSGEN's fourteen-file OS volume**.
They are copied as `.H99` Intel HEX files into the sibling `../MONITOR/`
upload-staging directory for transfer to the SBC as needed.

| Source | Staged file | Purpose | Entry address |
| --- | --- | --- | --- |
| `BDTEST62.A99` | `BDTEST62.H99` | BDOS 6.1 filesystem regression, allocation and directory tests | `>1000` (Shell COM area) |
| `IDE_DEBUG.A99` | `IDE_DEBUG.H99` | IDE/SATA interface diagnostics | `>0500` (Disc Monitor) |
| `MAPDEBUG.A99` | `MAPDEBUG.H99` | Paged memory-mapper diagnostics | `>0500` (Disc Monitor) |
| `MEMTEST.A99` | `MEMTEST.H99` | Banked/paged RAM integrity tests | `>0500` (Disc Monitor) |

## Build and upload

Run `MAKE_SYSTEM.bat` from the repository root on Windows. The master batch
stages the utility sources in `Build/`, assembles each with A99, checks for
successful `.H99` generation, then copies the four outputs into `../MONITOR/`
only after the system payload and all utilities have built successfully.
Upload an individual `.H99` using the Disc Monitor's `U` command. The three
monitor-oriented utilities are assembled to start at `>0500`; load and run
at that address (for example, `500G`). `BDTEST62` uses the Shell's `>1000`
transient program address and should be used in the normal Shell context.

**Caution:** The disk and memory diagnostic tools can write data to the SBC.
Read the source comments and select test targets carefully. BDTEST explicitly
uses `BDTEST.DAT` and `TESTDIR`; its fill test can consume available disk
space. IDE_DEBUG exposes drive-writing and erase commands, while MAPDEBUG
and MEMTEST can alter memory mapping and data.

`BDTEST62.A99` is also retained in `SRC/BDOS`; `IDE_DEBUG.A99` is also
retained in `SRC/DISC_MONITOR` for now. The copies here are what the master
build assembles. Keep them in sync when modifying these utilities.
