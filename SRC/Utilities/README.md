# Utilities — standalone programs for the TMS99105 SBC

The root `MAKE_SYSTEM.bat` assembles **every `.A99` in this folder** alongside
the operating system, but **does not include them in `PAYLOAD65.HEX` or the
SYSGEN volume**. A new utility only needs its source placed here, so don't keep
reference or scratch copies of `.A99` files in this folder.
It stages their individual Intel HEX (`.H99`) files in the sibling
`../STAGING` directory so that you can upload only the utility you need.

| Source | Output staged for upload | Description and usage |
| --- | --- | --- |
| [`BDTEST62.A99`](BDTEST62.A99) | `BDTEST62.H99` | [BDOS filesystem regression tests](README_BDTEST62.md) |
| [`IDE_DEBUG.A99`](IDE_DEBUG.A99) | `IDE_DEBUG.H99` | [IDE/SATA interface commands and disk-write cautions](README_IDE_DEBUG.md) |
| [`MAPDEBUG.A99`](MAPDEBUG.A99) | `MAPDEBUG.H99` | [Paged-memory mapper commands](README_MAPDEBUG.md) |
| [`MEMTEST.A99`](MEMTEST.A99) | `MEMTEST.H99` | [Automatic paged-RAM integrity tests](README_MEMTEST.md) |

## Build and loading

1. On Windows, run `MAKE_SYSTEM.bat` in the repository root. It stages the
   source files under `Build`, invokes A99 and checks that each `.H99` was
   generated successfully.
2. After the system image and all utilities have built successfully, it
   copies the utility `.H99` files and `PAYLOAD65.HEX` to `../STAGING`.
3. Upload the selected `.H99` with the SBC Disc Monitor's `U` command. Read
   the linked program guide before executing it.

The three hardware-oriented tools have an entry at `>0500` and are normally
started with the Disc Monitor's `500G` command; IDE_DEBUG also contains code
at `>1000`. BDTEST62 is a Shell/BDOS transient program assembled for `>1000`,
and it needs the Shell/BDOS environment to run and return correctly. An
`.H99` copied to PC staging is **not automatically a `.COM` file on disk**.

## Important cautions

- **BDTEST62** erases/recreates `BDTEST.DAT`, operates in `TESTDIR`, and can
  fill most of the filesystem with its `F` test.
- **IDE_DEBUG** can overwrite raw drive LBAs, including the boot region.
  Its command called VERIFY **writes** a pattern before reading it back.
- **MAPDEBUG** can alter mapper registers, modify RAM, enter endless
  diagnostic loops or transfer execution to mapped code.
- **MEMTEST** overwrites selected 4 KB RAM pages and returns through the
  Shell vector at `>0080`; it is not safe for occupied memory pages.

Each individual guide is based on the corresponding assembly source and
includes parameters, command/test descriptions and safety information.
Where the source's help text and implementation disagree, the guide calls
out the discrepancy instead of silently changing the source.

`BDTEST62.A99` is also retained in `SRC/BDOS`, and `IDE_DEBUG.A99` in
`SRC/DISC_MONITOR`, for historical compatibility. The copies **in this
directory** are the ones the master build assembles; keep them in sync when
modifying the utilities.
