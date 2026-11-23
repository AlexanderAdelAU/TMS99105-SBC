# OVL99 assembler demo (OVLTEST) — overlays written in assembler

The same overlay mechanism as the C demo one folder up, with every module written in
TMS99105 assembler. Use it as a template when an overlay, or the whole program, is
hand-written rather than compiled.

## What it shows

- The resident program calls `afunc1`, `bfunc1` and the rest **by name**. It has no
  overlay IDs, no page numbers, no fixed overlay addresses and no calls to OVLMGR.
  Only `CALL @ovlinit` knows overlays exist.
- **OVLA** (page 2) and **OVLB** (page 3) both run at `>8000`, so only one of them is
  mapped at a time.
- `bfunc2` in OVLB calls `afunc1` in OVLA. OVCALL maps OVLA, runs it, then maps OVLB
  back so `bfunc2` carries on in its own page.

## Contents

- `make_ovltest.ps1` — the Windows build
- `OVLMAP.TXT` — the overlay map
- `OVLTEST.LNK` — the link arguments, for both the Windows build and LINK99 on the SBC
- `src/`
  - `OVLTEST.A99` — the resident program, and `say`, a print routine the overlays share
  - `OVLA.A99`, `OVLB.A99` — the two overlays
- **Shared with the C demo:** the build takes these from `..` rather than keeping
  copies, so this folder holds no executables:
  - `..\tools\` — `r99.exe`, `link99.exe` 3.10.3, `pack99.ps1`
  - `..\runtime\` — `OVLMGR.A99`, `OVCALL.A99`
  - `..\src\CONOUT.A99` — `pstr()`

## Build on Windows

- In PowerShell:
  ```
  cd ovl99_demo\asm
  .\make_ovltest.ps1
  ```
- Output in `build\`: `OVLTEST.EXE`, and `OVLTEST.PAK` for linking on the SBC. Both are
  copied to your SBC `MONITOR` folder if `$env:SBC_MONITOR_DIR` is set, or a
  `TMS99105_SBC\MONITOR` folder is found above this one.
- The script passes r99 the drive the folder is on (`SCHC` on C:, `SDHD` on D:), so it
  builds from any drive.

## Run on the SBC

- XMODEM `OVLTEST.EXE` to the board, then `%OVLTEST`. It should print:
  ```
  OVL99 assembler overlay demo
  Test 1: afunc1 (maps OVLA)
    Overlay A - function 1
  Test 2: afunc2 (OVLA already mapped)
    Overlay A - function 2
  Test 3: bfunc1 (maps OVLB, then OVLA back)
    Overlay B - function 1
  Test 4: bfunc2 (OVLB calls into OVLA)
    Overlay B - function 2, calling A
    Overlay A - function 1
    Overlay B - back from A, my page is mapped again

  demo complete
  ```
- **Or link it on the SBC:** XMODEM `OVLTEST.PAK`, then `%UNPACK OVLTEST`,
  `%LINK99 @OVLTEST.LNK`, `%OVLTEST`.

## Writing an overlay in assembler

- **Place it:** `AORG 8000H` at the top (`9000H`.. for further pages of a multi-page
  overlay). The map gives the physical page.
- **Publish its entry points:** `ENT` each one, and list it in the map's entries
  column. The linker renames the overlay's copy `~name` and gives the resident thunk
  the plain name, so callers just `CALL @name` (with `EXT name`).
- **Calling convention:** the same as the C code's.
  - Enter by `CALL` (XOP 6), return with `RET` (XOP 7). The stack is `SP` (R10).
  - Arguments go on the stack: argument 1 is at `@2(SP)` on entry.
  - An overlay routine may change `R0`–`R3`, `R5` and `R12`. Anything else it
    changes it must save and restore.
- **Calling resident code:** directly, with `EXT`. The calling overlay stays mapped,
  so its own strings and data can be passed (as `say` does here).
- **Calling another overlay:** only through the other overlay's published name.
  Never pass it the address of your own overlay's data: the callee is mapped over
  it. Pass resident buffers.
- **Return mapping:** when a routine returns, OVCALL maps back the overlay that owned
  the window at the call. After Test 3 above, OVLA is mapped again although the
  program never asked for it.
- The rest of the rules, checked by the linker or yours to keep, are in the
  [C demo's README](../README.md#rules).

## What changed from the original OVLMGR proof of concept

The earlier version swapped one overlay into segment 2 by hand. It has been ported
to the current OVL99 runtime:

| Before | Now |
| --- | --- |
| Overlays at `>2000`, segment 2 | Overlays at `>8000`, the OVL99 window |
| Its own OVLMGR with a fixed page table | The shared OVL99 `OVLMGR` and `OVCALL`, with `OVL_TABLE` built by the linker |
| `LI R1,id` / `CALL @OVLMGR` / `CALL @>2000` | `CALL @afunc1`: the linker writes the thunk |
| Jump table at the top of each overlay | Each entry point is `ENT`ed and published in the map |
| A print routine reached through a vector at `>02A0` | `say`, a resident routine the overlays call by name |
| `link99 -P2 ... -P3 ...` | `link99 -O1000 -S -YOVLMAP.TXT`, as the C demo |
| `a99.exe`, `r99.exe`, `link99.exe`, `drel.exe` in the folder | The demo's shared `..\tools\` |
