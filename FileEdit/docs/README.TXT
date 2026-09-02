FILEEDIT -- overlay text editor and FILESTAT lineage (Step 7.1.6)
=================================================================

FILEEDIT began as FILESTAT, a TMS9900/TMS99105 text-file analyser built from
scratch around a resident core and mutually-exclusive >8000-8FFF overlays.
It has evolved into a full-screen Tera Term/VT100 editor while retaining the
original statistics commands.

CURRENT MILESTONE -- STEP 7.1.6
-------------------------------
Current hardware-proven functionality includes:

  full-screen editing with a display-only line-number gutter
  arrows, Home/End, PgUp/PgDn and rapid cursor movement
  insert, Backspace, Delete and Enter/line split/join
  literal ASCII TAB editing with four-column display tab stops
  Open, New, Save and Save As
  [MOD] dirty-document indication and unsaved-change protection
  rolling .BAK backup plus post-write verification
  original FILESTAT listing/statistics functionality
  five mapped code overlays, including OVL_EDIT
  64-byte interrupt-driven TMS9902 receive queue
  cooperative INT4 chaining to the previously installed handler

FILEEDIT.EXE is the primary name. FILESTAT.EXE is emitted as a byte-identical
compatibility alias for the original tutorial commands.

Commands
--------
    FILEEDIT SAMPLE.TXT        original analyser
    FILEEDIT SAMPLE.TXT /L     list then analyse
    FILEEDIT SAMPLE.TXT /V     read-only viewer
    FILEEDIT SAMPLE.TXT /E     editor with SAMPLE.TXT initially open
    FILEEDIT /E                editor with no initial document
    FILEEDIT /I                isolated INT4/TMS9902 timer-hook diagnostic

Editor keys
-----------
    Arrow keys       move cursor
    PgUp/PgDn        move by a screen
    Home/End         beginning/end of logical line
    Tab              insert a real ASCII 09 TAB
    Backspace        delete left
    Delete           delete under cursor
    Enter            split line
    Ctrl-O           Save + verify
    Ctrl-X           return to menu; document remains resident

Line-number gutter
------------------
The gutter is rendered by OVL_VIEW. Line numbers are not bytes in EDITBUF,
cannot be edited, are never saved, and do not affect FILESTAT statistics.

Resident/overlay split
----------------------
Resident modules hold persistent state, file I/O, terminal primitives, editor
buffer services and the TMS9902 RX queue. One overlay at a time occupies
>8000-8FFF:

    page 2  OVL_WORD
    page 3  OVL_CHAR
    page 4  OVL_RPT
    page 5  OVL_VIEW
    page 6  OVL_EDIT

DREL both generates overlay address tables and lints relocation chains.

TMS9902 receive path
--------------------
FILEEDIT temporarily replaces only the writable INT4 PC word at >0012 and
retains the MONITOR's INT4 workspace at >0010. CONRX.A99 is pure assembly and
must not use the Small-C stack: interrupt entry switches workspace, so the
application SP is not valid there.

RXISR explicitly loads R12=>0080, tests receive status with TB 21, inhibits /
acknowledges RX IRQ with SBZ 18, drains the byte with STCR, queues it, and
re-arms receive interrupts with SBO 18. It then ALWAYS branches to the saved
previous INT4 handler. The previous owner services timer/other INT4 work and
performs the final RTWP.

This prevents long screen redraws from losing bytes of VT100 key sequences.

See STEP7_1_INT4RX.TXT and ..\README.md.
