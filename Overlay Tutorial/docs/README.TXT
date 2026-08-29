FILESTAT
========
A small TMS99105 text-file utility that demonstrates a resident program with
paged overlays using the existing SBC toolchain.

What FILESTAT does
------------------
FILESTAT opens a text file, optionally lists it, runs two independent analysis
passes, and then prints a report.

    FILESTAT SAMPLE.TXT
    FILESTAT SAMPLE.TXT /L

/L (or -L) lists the file before analysis.

The program reports line, word, character, letter, digit, whitespace and
punctuation counts, plus the longest word found.

Project layout
--------------

    src/resident/FILESTAT.C   command line and program lifetime
    src/resident/FILEIO.C     open/read/rewind/list services
    src/resident/FILEDATA.C   persistent statistics and print helpers
    src/resident/OVLMGR.A99   physical-page mapper
    src/resident/OVLAPI.A99   C-callable overlay entry wrappers

    src/overlays/OVL_WORD.C   line/word/longest-word pass
    src/overlays/OVL_CHAR.C   character-class pass
    src/overlays/OVL_RPT.C    final report formatter

    include/OVLDEFS.INC       overlay IDs
    tests/SAMPLE.TXT          repeatable input
    tests/EXPECTED.TXT        expected statistics
    MEMMAP.TXT                virtual/physical memory map
    BUILDSBC.TXT              native build sequence
    build.ps1                 automated host-side build

Program flow
------------

    resident FILESTAT
          |
          +--> open file
          |
          +--> optional resident listing
          |
          +--> runword()
          |       map OVL_WORD at >8000
          |       wordscan()
          |       save results in resident FILEDATA
          |
          +--> rewind file
          |
          +--> runchar()
          |       map OVL_CHAR at the SAME >8000
          |       charscan()
          |       save results in resident FILEDATA
          |
          +--> close file
          |
          +--> runrpt()
                  map OVL_RPT at the SAME >8000
                  report() reads resident results

Why the design is split this way
--------------------------------
Resident code and data are always present.  An overlay is temporary: another
overlay may replace it at the same virtual address.  FILESTAT therefore keeps
persistent state in FILEDATA and FILEIO, while each overlay performs one phase
of work and returns.

The three overlays are mutually exclusive.  OVL_WORD, OVL_CHAR and OVL_RPT all
execute at >8000.  Loading one replaces whichever overlay was mapped there
previously.

Overlay changes are initiated by resident OVLAPI.A99.  An overlay may call
resident services, but it does not load or call another overlay directly.
This keeps ownership of the mapped window clear.

Never retain a pointer to data inside an overlay after that overlay returns.
The next page mapping may place unrelated contents at the same virtual address.
Persistent results must be copied to resident storage first.

Build modes
-----------
Exactly one C module is the executable entry module:

    FILESTAT.C      normal compilation

It receives the IOLIB startup branch.  Every other C source is a module:

    FILEIO.C        -M
    FILEDATA.C      -M
    OVL_WORD.C      -M
    OVL_CHAR.C      -M
    OVL_RPT.C       -M

-M suppresses the executable startup sequence and emits a linkable module.
BUILDSBC.TXT shows the native commands.

Overlay window
--------------
This introductory project uses one 4 KB virtual page, >8000->8FFF.  All three
overlays are assembled for that same address range but stored in different
physical pages.  OVLMGR selects which physical page is visible at >8000.

The build treats >0FC0 bytes as the maximum safe overlay payload, reserving the
top of the 4 KB page for linker chain requirements.  build.ps1 checks the DREL
size report and stops if an overlay exceeds that limit.

DREL-generated files
--------------------
DREL generates two include files used by the resident overlay support:

    OVLADDR.INC     virtual entry addresses used by OVLAPI
    OVLTABLE.INC    physical page assignments used by OVLMGR

They are generated during the build and are not edited by hand.

Toolchain
---------
FILESTAT uses the normal SBC chain:

    SMALLC99 / smallcp  -> .A99
    R99                 -> .R99
    DREL                -> overlay checks and generated include tables
    LINK99              -> FILESTAT.EXE and physical overlay pages

build.ps1 accepts -ToolRoot to locate these tools and libraries.  It does not
require any source files from another application project.

Naming constraints
------------------
The current toolchain supports short external identifiers, so FILESTAT keeps C
external names to eight characters or fewer.  The source/module filenames are
also 8.3-safe.

Suggested exercises
-------------------
- Add OVL_FREQ to produce a 26-letter frequency table.
- Add a resident option that selects which analysis passes run.
- Deliberately keep a pointer to overlay-local data, load another overlay, and
  observe why the pointer is no longer valid.
- Grow one overlay past the budget and observe the build stop before LINK99.
- Extend OVLMGR to use additional virtual segments in the >8000->BFFF window.
