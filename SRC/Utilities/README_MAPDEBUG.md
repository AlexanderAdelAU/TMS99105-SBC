# MAPDEBUG — paged-memory mapper diagnostics

Source: [`MAPDEBUG.A99`](MAPDEBUG.A99) · [Utilities overview](README.md)

`MAPDEBUG` exercises the SBC V4 memory mapper through the mapper registers
starting at `>80C0`. A CPU segment is a 4 KB virtual address window;
each segment selects a physical RAM page. The source uses Monitor XOPs
`PSEL_EN` and `PSEL_DIS` to enable/disable paged access around operations.

## Load and parameters

The build stages `MAPDEBUG.H99` in the sibling `../MONITOR` directory.
Upload it using the Disc Monitor `U` command, edit the parameter words
and execute **`500G`**. Each run performs one command and normally returns
to the Disc Monitor; it does not prompt for input.

| Word address | Parameter | Source default | Meaning |
| --- | --- | --- | --- |
| `>0502` | `COMMAND` | `>0003` | Command value (hex values below) |
| `>0504` | `SEG_PAGE` | `>0102` | High byte = page, low byte = segment (`>0102` means page 1, segment 2) |
| `>0506` | `VALUE` | `>A5A5` | Word to write; dump offset; or source page/segment for COPY |
| `>0508` | `OFFSET` | `>0000` | Byte offset for read/write commands 5 and 6 only |

For commands 5 and 6, the effective address is
`segment × >1000 + (OFFSET & >0FFF)`; the source masks the offset to the
4 KB window. For DUMP, the offset instead comes from `VALUE` at `>0506`.

## Commands

| Hex command | Name | Operation |
| --- | --- | --- |
| `1` | PSEN | Enable page select via XOP; the normal exit path disables it again |
| `2` | PSDIS | Disable page select |
| `3` | SMREG | Program the selected segment's mapper register and print readback |
| `4` | RMREG | Read the selected mapper register |
| `5` | WPGSEG | Write one word to the selected page/segment and offset |
| `6` | RPGSEG | Read one word from that address |
| `7` | CLR | Set all 16 segment mapper registers to page 0 |
| `8` | LST | List the 16 mapper-register values |
| `9` | RDLP | Continuously read from the selected mapped segment |
| `A` | PGLP | Repeatedly cycle all pages of a selected segment while reading |
| `B` | HLP | Print the on-board help |
| `C` | SETALL | Set segments 0–14 to matching page numbers and segment 15 to page 0 |
| `D` | SCYC | Repeatedly enable page select, read a word, disable it |
| `E` | INIT | Apply the same segment/page setup as SETALL |
| `F` | CODE | Copy a test routine to page 2/segment 2, plant `>BEEF` on page 3/segment 3, execute and check it |
| `10` | DUMP | Read words from a selected mapped address and display them |
| `11` | COPY | Copy **32 words (64 bytes)** between *different* segments/pages |
| `12` | GO | Map a selected page/segment and jump to its virtual segment base |

Commands `9`, `A` and `D` loop indefinitely and are labelled **RESET TO
EXIT** in the source. Command `12` transfers control to a program at
`segment × >1000`; it is not a diagnostic that returns normally.

**Source/help mismatch:** the DUMP comments and help say 64 bytes, but
the current implementation executes **16 lines × 8 words = 256 bytes**.
Take care when the starting offset is near the end of a 4 KB segment.
COPY rejects a source and destination with the same segment number; its
parameters are destination page/segment at `>0504` and source page/segment
at `>0506` (for example `>0602` and `>0503`).

## Safety and side effects

Reading a map register or printing HELP is relatively low risk; writing
memory or mapper registers is not. Commands `3`, `5`, `7`, `C`, `E`, `F`,
`11` and `12` can change mapper state or memory contents. CODE also
installs an interrupt-2 handler and executes code in mapped RAM. A normal
exit disables PSEL, **but does not restore all mapper registers to their
previous page selections**. Use a test configuration or save the register
values before experimenting; hardware reset may be necessary after loop
or code-execution commands.

