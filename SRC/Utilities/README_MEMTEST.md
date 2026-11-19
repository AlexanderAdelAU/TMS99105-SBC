# MEMTEST — banked/paged RAM integrity test

Source: [`MEMTEST.A99`](MEMTEST.A99) · [Utilities overview](README.md)

`MEMTEST` checks whether the SBC's 4 KB physical RAM pages retain distinct
contents when mapper registers are switched. Unlike MAPDEBUG, this is an
**automatic test** with no menu or run-time command parameters. Its source
still references the SHELLV62 mapper model; it is supplied here as a
standalone hardware test alongside the current Shell 6.5 build.

## Running it

The master build creates `MEMTEST.H99` and stages it under sibling
`../MONITOR`. The program assembles at **`>0500`**; upload with the Disc
Monitor `U` command and run at `500G` when the Shell is present. On normal
completion it uses **`BLWP @>0080`** to return through the Shell entry
vector. Do not assume a monitor-only environment without that Shell vector
can safely return from the test.

The source has fixed, build-time settings:

| Constant | Current value | Role |
| --- | --- | --- |
| `SEG_LO` / `SEG_HI` | 2 / 3 | Virtual 4 KB windows at `>2000` / `>3000` |
| `PAGE_P` / `PAGE_Q` | 2 / 3 | Physical page selections used by the tests |
| `TESTLEN` | `>1000` bytes | Full 4 KB per tested window |
| `WCOUNT` | `>0800` words | 2,048 16-bit words per pattern |
| `MAP_WIN_BASE` | `>80C0` | Mapper-register interface |
| `SEED` | `>ACE1` | Seed for the pseudo-random LFSR pattern |

## What it tests

1. **PROBE:** Set/read mapper registers and plant three sentinels:
   `AA55` and `1234` on segment 2/page P, and `5678` on segment 2/page Q.
   It prints the observed values for inspection.
2. **S1 BANK:** Fill segment 2/page P, switch segment 2 to page Q and back,
   then verify the original pattern survived.
3. **S2 MOVE:** Fill segment 2/page P, copy all 4 KB into segment 3/page Q,
   cycle both mapper entries and verify the source and destination independently.
4. **S3 ISOLATE:** Write a pattern to segment 2/page P and its complement
   to segment 2/page Q; switch between them and verify both independently.

Each scenario uses six patterns: all zeroes, all ones, `AA55`, byte-address
offset, complement of address offset, and a 16-bit LFSR sequence. There are
**24 reported PASS/FAIL checks**: six in S1, six in S2, and twelve in S3.
The initial PROBE values are printed separately and are not included in
those totals.

The output ends with `SUMMARY pass=... fail=...` and `MEMTEST COMPLETE`.
Failures show the first failing byte offset, expected word and actual word.

## Safety — RAM is overwritten

**This is a destructive memory test.** It writes entire 4 KB windows on
pages 2 and 3 of segments 2 and 3, plus probe sentinels. It saves and
restores the original mapper-register selections for those two segments
on its normal completion path, but **does not restore the original RAM
contents**. Do not run it while those physical pages contain live code,
buffers or user data. An interruption may also prevent mapper restoration.

The hardware model assumes pages from different segments do not alias:
segment 2/page 2 and segment 3/page 2 are not the same 4 KB physical
region, even though both select page number 2.

