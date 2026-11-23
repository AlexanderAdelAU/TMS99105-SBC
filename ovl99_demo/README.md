# OVL99 demo (OVLDEMO) — building an overlay application for the TMS99105 SBC

The smallest complete OVL99 program: a C `main()` that calls functions living in
three overlay modules, as if they were ordinary functions. Use it as a template for
your own overlay application.

## What it shows

- `main()` calls `hello()` and `farewell()` with no overlay code at all — no IDs, no
  mapping calls, no externs. Only `ovlinit()` knows overlays exist.
- **GREET** is a two-page overlay (`>8000` and `>9000`, physical pages 4 and 6); its
  pages call each other directly.
- **BYE** (`>8000`, page 5) shares segment 8 with GREET, so only one of them is
  mapped at a time.
- `count()` in BYE calls back into GREET while GREET's `hello()` is still waiting:
  OVCALL's frame stack maps each caller's overlay back as the calls return.

## Contents

- `make_ovldemo.ps1` — the Windows build
- `OVLMAP.TXT` — the overlay map: the one place the overlays are declared
- `OVLDEMO.LNK` — the link arguments, used by both the Windows build and LINK99 on the SBC
- `src/`
  - `MAIN.C` — the application (resident)
  - `OVLHELLO.C`, `OVLGRET2.C` — overlay GREET, pages 1 and 2
  - `OVLBYE.C` — overlay BYE
  - `STARTUP.A99`, `CONOUT.A99` — a minimal C start-up and `pstr()`, so the demo
    needs no file I/O. A real application links the normal C run-time instead.
- `runtime/` — the OVL99 runtime, linked unchanged into every overlay program
  - `OVLMGR.A99` — maps one overlay's row of `OVL_TABLE`
  - `OVCALL.A99` — the call gate, and `ovlinit()`
- `lib/` — `CALL.R99` and `CLIB99.LIB`/`.NDX` from the C run-time
- `tools/` — `smallcp.exe` (Small-C compiler), `r99.exe` (assembler),
  `link99.exe` 3.10.3 (linker), `pack99.ps1`
- `asm/` — the same mechanism with every module written in assembler (OVLTEST),
  built with this folder's `tools/` and `runtime/`

## Build on Windows

- In PowerShell:
  ```
  cd ovl99_demo
  .\make_ovldemo.ps1
  ```
- Output in `build\`:
  - `OVLDEMO.EXE` — the program
  - `OVLDEMO.PAK` — all the link inputs in one file, for linking on the SBC
- Both are copied to your SBC `MONITOR` folder if `$env:SBC_MONITOR_DIR` is set, or a
  `TMS99105_SBC\MONITOR` folder is found above this one.

## Run on the SBC

- XMODEM `OVLDEMO.EXE` to the board (XMODEM sets the file type to EXE from the name),
  then:
  ```
  %OVLDEMO
  OVL99 overlay demo
    hello:    in overlay GREET
    greet:    in overlay GREET
    shout:    GREET page 2, reached by thunk with no mapping
    greet:    in overlay GREET
    count:    in overlay BYE, called from GREET
    greet:    in overlay GREET
    count:    back from GREET, my page was mapped again
    hello:    back from BYE, my page was mapped again
    farewell: in overlay BYE
  demo complete
  ```

## Or link it on the SBC

- Needs `LINK99.EXE` and `UNPACK.COM` on the board (from the TMS99105-Native-Toolchain
  repository: `link99` and `pack99`).
- XMODEM `OVLDEMO.PAK` across, then:
  ```
  %UNPACK OVLDEMO
  %LINK99 @OVLDEMO.LNK
  %OVLDEMO
  ```
- LINK99 reads the same map and writes the same bytes as the Windows link99.

## How it works

- **The map** (`OVLMAP.TXT`) gives each overlay module its physical page, its
  overlay name, and the functions other modules may call:
  ```
  OVLHELLO      4     GREET    hello greet
  OVLGRET2      6     GREET    shout
  OVLBYE        5     BYE      farewell count
  ```
- **The segment** comes from each module's own `AORG`, at the top of the C file:
  ```c
  #asm
          AORG 8000H
  #endasm
  ```
- **The compiler** builds overlays as modules: `smallcp -C -M OVLHELLO`.
- **The linker** (`link99 -O1000 -YOVLMAP.TXT ...`):
  - loads each overlay module on its map page
  - renames each published function `hello` → `~hello`
  - writes a 12-byte resident **thunk** named `hello`: `LI R1,id / LI R2,~hello / B @OVCALL`
  - writes `OVL_TABLE`: one row of (segment, page) pairs per overlay
  - checks the rules below, and refuses the link if one is broken
- **At run time**, a call to `hello()` lands in the thunk. `OVCALL`:
  - takes the fast path if GREET already owns the window: a plain branch
  - otherwise pushes (return address, current owner), maps GREET, calls `~hello`,
    then maps the caller's whole overlay back and returns
- **The EXE** is a paged chain: the shell's loader maps each block's page as it loads,
  and the heap ends below `>8000`, the lowest overlay address.

## Adding an overlay of your own

- Put the code in its own C file with `AORG 08000H` at the top (or `9000H`.. for
  further pages of a multi-page overlay).
- Add a line to the map: module name, a free page (1–15), an overlay name, the
  functions other modules call.
- Add the `.R99` to `OVLDEMO.LNK` after the resident modules, and to the compile and
  assemble lists in `make_ovldemo.ps1`.
- Call it like any other function.

## Rules

- **Checked by the linker:**
  - every overlay maps segment 8 first (the window's owner)
  - an overlay's segments are adjacent and inside `>8000`–`>BFFF`
  - no (segment, page) pair is used twice
  - each module stays inside its 4K page
  - nothing resident lands above the overlays
- **Yours to keep:**
  - never call a function in another overlay except by its published name
  - never pass a pointer to an overlay's own data (a string literal, a static array)
    to a function in another overlay — the callee can map over it. Pass resident
    buffers or locals.
  - call `ovlinit()` first in `main()`
  - nest at most 16 cross-overlay calls (`>DEAD` is printed beyond that)

## More

- `OVL99.md` in the TMS99105-Native-Toolchain repository — the full reference:
  runtime, table layout, map format (`pub=routine`, `:tail`, `guest`, `window`)
- SmallC99 and FileEdit — two large programs built the same way
