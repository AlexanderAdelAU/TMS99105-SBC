# BDTEST62 — BDOS filesystem regression test

Source: [`BDTEST62.A99`](BDTEST62.A99) · [Utilities overview](README.md)

`BDTEST62` exercises the filesystem through the BDOS API. It checks
sequential and random I/O, allocation boundaries, File Control Block (FCB)
state, end-of-file behaviour, and folder isolation. The program banner still
calls this a BDOS 6.0 test; this repository builds it alongside **BDOS61**.

## Running it

- The source is assembled at **`>1000`**, the Shell transient-program area.
- `MAKE_SYSTEM.bat` creates `BDTEST62.H99` in `Build` and copies it to the
  sibling `../MONITOR` upload-staging directory. It is **not** in SYSGEN.
- Load the Intel HEX image with the Disc Monitor `U` command, then run at
  `>1000` **with the Shell and BDOS environment present**. Alternatively,
  install it as a Shell-loadable program on the SBC disk. Staging an `.H99`
  on the PC does not itself install a `.COM` file on the SBC disk.
- The source reads the Shell command-line buffer at `>00A0`: an optional
  sequence of menu letters can be supplied after the program name. With no
  option it displays an interactive menu. Options are case-insensitive.

## Menu

| Option | Test |
| --- | --- |
| `S` | Write, close, reopen, read and delete a **5-record** file |
| `B` | Same lifecycle over **505 records**, crossing the physical-block 127/128 boundary |
| `P` | **1,017 records**, crossing the BAT page 0/1 boundary |
| `2` | **2,041 records**, crossing the BAT page 1/2 boundary |
| `F` | Fill-capacity test, targeting up to **32,704 records**; also checks a further write is rejected when full |
| `R` | Random record reads, overwrite, append, sparse/EOF rejection and FCB cursor/size checks |
| `C` | Sequential cursor repositioning and subsequent read consistency |
| `D` | Create/enter `TESTDIR`, test a file there, then return to the root directory on success |
| `A` | Run `S`, `B`, `P`, `2`, `R`, `C` and `D` in sequence; **excludes `F`** |
| `Q` | Exit to the Shell |

Each normal file test creates `BDTEST.DAT`, fills 512-byte records with a
recognisable word pattern and record markers, verifies the contents and FCB
bookkeeping after reopening, and erases the test file. The disk geometry in
the source assumes 4 KB allocation blocks, system blocks 0–7, and a
16.77 MB disk with up to 32,704 data records.

## Reading results

- Success prints messages such as `WRITE/CAPACITY CHECK OK`,
  `READ/PATTERN/EOF CHECK OK`, `DELETE/RECLAIM OK`, and
  `*** TEST PASSED ***`.
- Failure prints a numeric `CODE`, record, BDOS status and relevant FCB or
  data mismatch fields (`FSB`, `FSZ`, `CRN`, byte offset, expected, actual),
  followed by a warning to preserve the disk image before retrying.
- The `D` test returns to root on its successful path; a failure while
  operating in `TESTDIR` may leave the current folder changed.

## Safety — this test writes to disk

**Back up your SBC disk before running this program.** `CREATE_FILE`
explicitly erases an existing `BDTEST.DAT` before recreating it, so do not
keep user data under that filename. The `D` test creates/uses `TESTDIR`;
it does not remove the directory at the end. `F` can consume essentially
all available disk space and is deliberately excluded from `A`. If any
test fails, inspect the disk state before rerunning it.

