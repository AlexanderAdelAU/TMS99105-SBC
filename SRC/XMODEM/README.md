# XMODEM59 — file receiver for the TMS99105 SBC

`XMODEM59.A99` implements the **receiving** side of the XMODEM protocol
on the SBC. It transfers a file from a terminal program on the host PC
into the SBC disk filesystem through BDOS. The operating-system build
packages the assembled program as `XMODEM.COM`.

## Usage

At the Shell prompt, enter:

```text
XMODEM <FILENAME> [-HHHH]
```

- `FILENAME` is the destination file on the SBC (for example,
  `DEMO.COM` or `NOTES.TXT`).
- `-HHHH` is an optional load address of **one to four hexadecimal
  digits**. The default is `1000` (the transient program area).
- The switch records the load address in the file's directory/FCB
  metadata; it does **not** relocate the file's contents.

Examples:

```text
XMODEM DEMO.COM
XMODEM DEMO.COM -0500
XMODEM NOTES.TXT
```

After starting XMODEM on the SBC, use your terminal application's
**XMODEM Send** command to send the selected file from the PC. The SBC
receives it and writes it to disk using BDOS.

**Important:** The program first erases an existing destination file
with the same name before creating the new one. Choose the filename
carefully—this is an overwrite operation.

## Supported transfer behaviour

The implementation:

- Requests **CRC-16** initially by sending the `C` character, then
  falls back to the original one-byte checksum handshake if the
  sender does not respond.
- Accepts both standard **128-byte packets** (`SOH`) and
  **1,024-byte packets** (`STX`, XMODEM-1K).
- Validates the block number, its complement and the CRC/checksum.
- Acknowledges repeated packets without writing duplicate data,
  and rejects out-of-sequence packets.
- Buffers received data into 512-byte disk writes through BDOS,
  flushing the remaining records when the transfer finishes.
- Acknowledges the end-of-transmission marker (`EOT`); on a fatal
  transfer/write error it sends a cancel character and attempts
  to erase the incomplete destination file.

The source also derives the SBC file type from common extensions:
`COM`, `EXE`, `TXT`, `BAS` and `PRO`. Unrecognised extensions
fall back to the `SYS` type code.

## Building and dependencies

The maintained source is:

```text
SRC/XMODEM/XMODEM59.A99
```

Run `MAKE_SYSTEM.bat` from the repository root on drive C:. The build
assembles this file to `Build/XMODEM59.H99`, which SYSGEN packages as
`XMODEM.COM`. XMODEM depends on the SBC's ROM monitor serial services
and on BDOS filesystem functions.

It is **not** part of the ROM Disc Monitor: the latter is maintained
separately under `SRC/DISC_MONITOR` and is not included in the SYSGEN
operating-system payload.

## Source notes

The header of `XMODEM59.A99` records the incremental development of
CRC reception, optional load addresses, XMODEM-1K packets and filename
validation. These features are present in the current source even though
the distributed filename remains `XMODEM59.A99`.
