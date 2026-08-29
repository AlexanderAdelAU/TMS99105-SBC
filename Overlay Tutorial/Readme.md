# FILESTAT — TMS99105 SBC Paged Overlay Example

`FILESTAT` is a small text-file analyser written as a worked example of how to build and use **paged overlays** on the TMS99105 SBC.

The program itself is deliberately simple. It opens a text file, optionally lists it, performs two analysis passes, and prints a report. The useful part is the structure: resident code and data remain permanently available while several independent overlay modules reuse the same virtual memory window.

The example uses the existing, proven SBC overlay infrastructure rather than reimplementing the mapper.

## What it demonstrates

FILESTAT shows the complete application-side overlay pattern:

- one normal executable module owns startup;
- resident helper modules remain permanently mapped;
- overlay C modules are compiled with `-M`;
- overlays are assembled for the same virtual address;
- DREL generates the overlay entry-address and page tables;
- the standard `OVLMGR.A99` performs the page mapping;
- a thin resident `OVLAPI.A99` provides C-callable overlay entry points;
- persistent results are stored in resident memory, never in an overlay;
- LINK99 places each overlay in its assigned physical page.

The central rule is:

```text
code that may be replaced  -> overlay
state that must survive    -> resident
```

## Example

On the SBC the executable can be given whatever local filename is convenient. The examples below use `FILE99`.

```text
%FILE99 EXAMPLE.TXT /L

--- file contents ---
Overlay systems reuse memory.

Resident data survives overlay swaps.

Numbers 123 are counted too.

--- end contents ---

FILESTAT report
File: EXAMPLE.TXT
Lines: 3
Words: 14
Characters: 97
Letters: 77
Digits: 3
Whitespace: 14
Punctuation: 3
Longest word: Resident (8)
%
```

Without `/L` the file is analysed without first being listed:

```text
%FILE99 EXAMPLE.TXT
```

`/L` and `-L` are both accepted.

# Screen Shot
<p align="center">
  <img src="Screenshot.jpg" alt="Screenshot" width="600">
</p>

## Program architecture

```text
                         RESIDENT
             >1000 ---------------------- >7FFF

                         FILESTAT
                            |
                            |  startup
                            v
                       OVLMGR_INIT
                            |
                            v
                         fsopen()
                            |
                 +----------+----------+
                 |                     |
              optional                  |
               fslist()                 |
                 |                     |
              fsrewind()                |
                 |                     |
                 +----------+----------+
                            |
                            v
                         runword()
                            |
                      CALL @OVLMGR
                            |
                     map physical
                       page 2 at
                         >8000
                            |
                            v
                       wordscan()
                     [ OVL_WORD ]
                            |
                  resident counters
                            |
                            v
                        fsrewind()
                            |
                            v
                         runchar()
                            |
                      CALL @OVLMGR
                            |
                     map physical
                       page 3 at
                         >8000
                            |
                            v
                       charscan()
                     [ OVL_CHAR ]
                            |
                  resident counters
                            |
                            v
                        fsclose()
                            |
                            v
                         runrpt()
                            |
                      CALL @OVLMGR
                            |
                     map physical
                       page 4 at
                         >8000
                            |
                            v
                         report()
                      [ OVL_RPT ]
```

The three overlay modules are mutually exclusive. At any instant the virtual address range beginning at `>8000` contains whichever overlay is currently selected.

## Memory map

FILESTAT uses only the first 4 KB of the standard overlay window:

```text
>1000  +--------------------------------------------------+
       | Resident application and runtime                 |
       |                                                  |
       | FILESTAT   command line / lifetime control       |
       | FILEIO     persistent file services              |
       | FILEDATA   persistent statistics                 |
       | OVLMGR     standard shared overlay manager       |
       | OVLAPI     FILESTAT overlay adapters             |
       | IOLIB / CLIB                                     |
>7FFF  +--------------------------------------------------+

>8000  +--------------------------------------------------+
       | FILESTAT overlay window                          |
       |                                                  |
       | OVL_WORD  or  OVL_CHAR  or  OVL_RPT             |
>8FFF  +--------------------------------------------------+

>9000  +--------------------------------------------------+
       | Unused by this example                           |
       |                                                  |
       | The standard manager supports the larger         |
       | >8000-BFFF overlay window when required.         |
>BFFF  +--------------------------------------------------+

>C000  +--------------------------------------------------+
       | System / shell / monitor                         |
>FFFF  +--------------------------------------------------+
```

Physical page assignments used by this example are:

| Overlay | Overlay ID | Physical page | Virtual entry area |
|---|---:|---:|---|
| `OVL_WORD` | 1 | 2 | `>8000` |
| `OVL_CHAR` | 2 | 3 | `>8000` |
| `OVL_RPT` | 3 | 4 | `>8000` |

All three C overlays contain:

```c
#asm
        AORG    08000H
#endasm
```

so they are linked to execute in the same virtual window even though LINK99 stores them in different physical pages.

## Project layout

```text
FILESTAT/
|
+-- src/
|   +-- resident/
|   |   +-- FILESTAT.C      program control and command line
|   |   +-- FILEIO.C        open/read/rewind/list services
|   |   +-- FILEDATA.C      persistent counters and longest word
|   |   +-- OVLMGR.A99      standard shared overlay manager
|   |   `-- OVLAPI.A99      FILESTAT-specific overlay adapters
|   |
|   `-- overlays/
|       +-- OVL_WORD.C      line/word/longest-word pass
|       +-- OVL_CHAR.C      character classification pass
|       `-- OVL_RPT.C       report formatter
|
+-- include/
|   `-- OVLDEFS.INC         FILESTAT overlay IDs
|
+-- tests/
|   +-- SAMPLE.TXT
|   `-- EXPECTED.TXT
|
+-- tools/                   project-local SBC build tools
+-- lib/                     project-local runtime objects/libraries
+-- build/                   disposable staging/build directory
+-- dist/                    final deliverables
|
+-- MEMMAP.TXT
+-- BUILDSBC.TXT
`-- make_SBC_filestat99.ps1
```

## The standard overlay manager

`OVLMGR.A99` is shared SBC infrastructure. FILESTAT uses the standard manager **unchanged**.

Applications should not duplicate or simplify the page-mapping code. Instead, they call the public interface already provided by the manager.

At startup:

```asm
        CALL    @OVLMGR_INIT
```

Before entering an overlay:

```asm
        LI      R1,overlay_id
        CALL    @OVLMGR
        CALL    @overlay_entry
```

The standard manager:

1. looks up the overlay ID in the DREL-generated table;
2. determines the virtual segment and physical page;
3. updates the SBC mapping registers;
4. enables paged selection;
5. returns to the resident caller.

The current manager supports the full `>8000-BFFF` window. FILESTAT intentionally uses only segment 8 (`>8000-8FFF`) so the example remains easy to follow.

## Initialise paging before normal application work

FILESTAT calls its resident `ovlinit()` adapter immediately on entry:

```c
main(argc, argv)
int argc;
char **argv;
{
    ovlinit();

    /* normal application code follows */
}
```

The adapter simply calls the standard manager:

```asm
ovlinit:
        CALL    @OVLMGR_INIT
        RET
```

This establishes the normal overlay environment before the application opens files or performs other work.

## Calling convention

Small-C/Plus-generated TMS9900 code uses the monitor XOP calling convention:

```asm
        DXOP    CALL,6
        DXOP    RET,7
```

Overlay adapters must therefore use `CALL`/`RET` when calling C functions.

The FILESTAT wrapper for the word-analysis overlay is:

```asm
runword:
        LI      R1,OVL_WORD
        CALL    @OVLMGR
        CALL    @O_WORD
        RET
```

The same pattern is used for `runchar` and `runrpt`.

Do not substitute a normal TMS9900 `BL` call for a Small-C function that returns with XOP7 `RET`. They are different calling conventions.

## Resident state

The most important application-design rule is that an overlay is temporary.

For example, `OVL_WORD` calculates:

- line count;
- word count;
- longest word.

Those results must still exist after `OVL_CHAR` replaces `OVL_WORD` at `>8000`, so the values live in resident `FILEDATA.C`.

Likewise, `FILEIO.C` owns the persistent file state used by both analysis overlays.

An overlay may call resident services:

```text
OVL_WORD -> fsgetc()
OVL_WORD -> noteword()

OVL_CHAR -> fsgetc()

OVL_RPT  -> putstat()
OVL_RPT  -> puts()
```

but an overlay should not directly load or call another mutually exclusive overlay.

Also, never retain a pointer to overlay-local code or data after switching overlays. The same virtual address may immediately refer to unrelated contents.

## Overlay IDs

FILESTAT keeps its application-specific overlay IDs in `include/OVLDEFS.INC`:

```asm
OVL_WORD    EQU 1
OVL_CHAR    EQU 2
OVL_RPT     EQU 3
```

ID zero is unused.

The application code needs to know only the logical overlay ID. Physical page placement is supplied to DREL by the build.

## DREL-generated files

The build does not hand-code overlay entry addresses or mapper table contents.

DREL generates:

```text
OVLADDR.INC     symbolic virtual entry addresses
OVLTABLE.INC    overlay ID -> segment/page mapping table
```

For this project the build descriptors are:

```text
OVL_WORD.R99=2,1,wordscan:O_WORD
OVL_CHAR.R99=3,2,charscan:O_CHAR
OVL_RPT.R99=4,3,report:O_RPT
```

They define the physical page, overlay ID, exported C entry point, and resident alias used by `OVLAPI.A99`.

For example, DREL derives the actual virtual address of `wordscan` and emits it as `O_WORD` in `OVLADDR.INC`. `OVLAPI.A99` therefore does not need a hand-maintained address.

The generated `.INC` files are build products. Do not edit them manually.

## Build modes

Exactly one C module is compiled as the executable entry module:

```text
FILESTAT.C
```

All resident helper modules and overlays are compiled with `-M`:

```text
FILEIO.C
FILEDATA.C
OVL_WORD.C
OVL_CHAR.C
OVL_RPT.C
```

`-M` suppresses executable startup generation and produces a module suitable for linking into the application.

## Toolchain

The project uses the normal SBC toolchain:

```text
Small-C/Plus
     |
     v
   .A99
     |
    R99
     |
     v
   .R99
     |
    DREL
     |
     +----> OVLADDR.INC
     |
     +----> OVLTABLE.INC
     |
     v
  LINK99
     |
     v
FILESTAT.EXE
```

The build script uses:

```text
smallcp.exe
r99.exe
drel.exe
link99.exe
```

together with the project-local runtime objects and libraries under `lib/`.

## Building

The PowerShell build is deliberately self-contained.

From the project directory:

```powershell
.\make_SBC_filestat99.ps1
```

The source tree is authoritative. At the start of every build the script deletes and recreates `build/`, then copies all required tools, libraries, source files, includes and the sample input into that directory.

Every compile, assembly, DREL and LINK99 operation is then run from `build/`.

The flow is:

```text
src/ + include/ + tools/ + lib/
              |
              | stage
              v
           build/
              |
              | Small-C/Plus
              | R99
              | DREL
              | R99 OVLMGR/OVLAPI
              | LINK99
              v
        FILESTAT.EXE
              |
              +----> dist/
              |
              `----> monitor/download directory
```

The build is anchored to `$PSScriptRoot`, so it does not depend on Eclipse's current working directory.

## Build sequence

The automated script performs the following steps.

### 1. Compile C

Normal executable:

```text
smallcp -C FILESTAT
```

Resident and overlay modules:

```text
smallcp -C -M FILEIO
smallcp -C -M FILEDATA
smallcp -C -M OVL_WORD
smallcp -C -M OVL_CHAR
smallcp -C -M OVL_RPT
```

### 2. Assemble generated source

For example:

```text
r99 FILESTAT SCHCLC
r99 FILEIO SCHCLC
r99 OVL_WORD SCHCLC
```

The remaining modules are assembled the same way.

### 3. Check overlay REL chains

```text
drel -c OVL_WORD.R99 OVL_CHAR.R99 OVL_RPT.R99
```

The build stops unless all chains conform.

### 4. Generate overlay metadata

DREL generates `OVLADDR.INC` and `OVLTABLE.INC` from the overlay descriptors.

### 5. Assemble resident overlay support

Only after the generated include files exist:

```text
r99 OVLMGR SCHCLC
r99 OVLAPI SCHCLC
```

### 6. Check overlay sizes

This tutorial assigns one physical 4 KB page to each overlay. The build enforces a chain-safe payload limit of:

```text
>0FC0 bytes
```

for each single-page overlay.

### 7. Link

LINK99 receives the resident modules plus the physical page assignments:

```text
-P2 OVL_WORD.R99
-P3 OVL_CHAR.R99
-P4 OVL_RPT.R99
```

The final executable contains the resident code and the overlay page images.

## Adding another overlay

A useful exercise is to add an `OVL_FREQ` module that calculates a letter-frequency table.

The application-side procedure is:

1. Create `src/overlays/OVL_FREQ.C`.
2. Compile it with `-M`.
3. Assemble it for the required virtual address, normally beginning with `AORG >8000` for a single-page overlay.
4. Give it a unique logical ID in `OVLDEFS.INC`.
5. Assign a physical page in the build script.
6. Add its DREL descriptor, including the exported entry function and resident alias.
7. Let DREL regenerate `OVLADDR.INC` and `OVLTABLE.INC`.
8. Add a thin resident wrapper in `OVLAPI.A99`.
9. Keep any results needed after the overlay returns in resident storage.
10. Rebuild and let the existing DREL/LINK99 checks validate the new module.

A wrapper follows exactly the same pattern:

```asm
runfreq:
        LI      R1,OVL_FREQ
        CALL    @OVLMGR
        CALL    @O_FREQ
        RET
```

The important point is that the application adds **policy and functionality**, not a new mapper.

## Practical overlay rules

When developing an overlay application for the SBC:

1. **Use the standard `OVLMGR.A99` unchanged.**
2. **Call `OVLMGR_INIT` once during startup.**
3. **Use Small-C's XOP6 `CALL` / XOP7 `RET` convention for C functions.**
4. **Keep persistent state resident.**
5. **Do not keep pointers into an overlay after another overlay may be loaded.**
6. **Let DREL generate overlay addresses and mapping tables.**
7. **Do not hard-code entry addresses that DREL already knows.**
8. **Enter overlays through resident wrappers.**
9. **Do not directly call between mutually exclusive overlays.**
10. **Treat `build/` as disposable; edit only the authoritative source tree.**

## Why FILESTAT is split into three overlays

The split is intentionally visible rather than optimized away.

`OVL_WORD` demonstrates an overlay that:

- loops over resident file I/O;
- maintains local working storage;
- writes persistent results into resident data.

`OVL_CHAR` demonstrates replacing the first overlay with unrelated code while retaining all previous results.

`OVL_RPT` demonstrates that an overlay can consume resident results produced by earlier overlays after those overlays no longer exist in the virtual window.

That makes FILESTAT a small but complete demonstration of the lifetime rules involved in paged overlay software.

## Test data

`tests/SAMPLE.TXT` has the expected statistics:

```text
Lines: 3
Words: 14
Characters: 97
Letters: 77
Digits: 3
Whitespace: 14
Punctuation: 3
Longest word: Resident (8)
```

A word is a run of:

```text
A-Z
a-z
0-9
_
```

Character counts are based on the logical text stream delivered by IOLIB. Its input layer normalizes the supported line-ending forms before FILESTAT sees them.

## Naming

The source and external identifiers are kept short for compatibility with the SBC toolchain.

The development executable is `FILESTAT.EXE`. If the target filesystem or monitor uses a different local naming convention, rename the downloaded executable as required; the example sessions above use `FILE99`.

## Summary

The reusable pattern is intentionally small:

```text
                     once at startup
                           |
                           v
                    OVLMGR_INIT
                           |
                           v

resident C -> resident wrapper -> standard OVLMGR -> overlay C
                                      |
                                      v
                               generated table

overlay C -> resident services/data -> survives next page swap
```

Once that boundary is respected, the application does not need to know how the physical mapper works. It selects an overlay by ID, calls its entry point, and keeps anything that must survive in resident memory.

That is the model FILESTAT is intended to demonstrate.

