# IDE_DEBUG — IDE/SATA interface diagnostics

Source: [`IDE_DEBUG.A99`](IDE_DEBUG.A99) · [Utilities overview](README.md)

This is a low-level IDE/SATA diagnostic for the SBC V4's **8-bit drive
interface** at I/O base `>8040`. It issues device commands directly; it is
not a BDOS file utility. One physical IDE LBA transfers **256 bytes** on
this interface, whereas a BDOS sector is **512 bytes** (two IDE LBAs).

## Load and select a command

The generated `IDE_DEBUG.H99` is staged in the sibling `../MONITOR` directory
and is not included in SYSGEN. Upload it with the Disc Monitor `U` command.
It has a parameter block and entry trampoline at **`>0500`**, while the
main routine is assembled at **`>1000`**. Edit the command/parameters in
memory, then execute `500G` in the Disc Monitor. It is **not an interactive
menu**: change `>0502` and rerun for the next operation.

| Word address | Parameter | Source default | Meaning |
| --- | --- | --- | --- |
| `>0502` | `COMMAND` | `>0001` | Operation selector |
| `>0504` | `LBA_HI` | `>0000` | High word of the selected drive LBA |
| `>0506` | `LBA_LO` | `>0001` | Low word of the LBA; also used as block number by RDBLOCK |
| `>0508` | `BUFFER` | `>2000` | RAM buffer address |
| `>050A` | `SECTORS` | `>0001` | Count for the VERIFY command (1–20) |

## Commands (values to put at `>0502`)

| Value (hex) | Source command | Action | Disk writes? |
| --- | --- | --- | --- |
| `>0000` | MEMDSK | Write 32 IDE LBAs starting at **LBA 0** from RAM starting at `>1000`, then read/compare | **Yes — overwrites boot/system area** |
| `>0001` | STATUS | Read status bits (BSY, DRDY, DRQ, ERR, etc.) and error register | No |
| `>0002` | RDLBA | Read one 256-byte LBA into `BUFFER` | No |
| `>0003` | WRLBA | Write the 256-byte contents of `BUFFER` to the selected LBA | **Yes** |
| `>0004` | DRVID | Issue IDENTIFY DEVICE and retain the result in `BUFFER` | No |
| `>0005` | ERASE | Write 32 consecutive LBAs with `E5` bytes, starting at `LBA_LO` | **Yes** |
| `>0006` | DUMP | Print the current 256-byte buffer as words | No |
| `>0007` | HELP | Show the built-in command help | No |
| `>0008` | INFO | Identify the drive and decode model, serial, firmware and geometry | No |
| `>0009` | VERIFY | **Write** an `A5` test pattern to 1–20 LBAs and read it back for comparison | **Yes** |
| `>0010` | INIT | Issue software reset and Initialise Drive Parameters (`>91`) | Changes drive state |
| `>0011` | RDBLOCK | Read and dump one 4 KB BDOS block (16 physical LBAs) | No |
| `>000C` | DISKMAP | Print the compiled-in boot/BAT/directory/alias/data LBA ranges | No |

**Important source discrepancy:** the program banner calls its last command
`12=DISKMAP`, but `CMD_DISKMAP EQU 12` has **no `H` suffix**, so the
assembled selector is decimal 12 (`>000C`), not `>0012`. In contrast,
INIT and RDBLOCK use `10H` and `11H`. This guide reports what the source
actually dispatches; the program has not been changed.

For RDBLOCK, set `>0506` to the allocation-block number. The routine
multiplies it by 16 to obtain the first IDE LBA and reads 16 × 256-byte
sectors. DISKMAP prints a **static map** from the source; it does not scan
the current drive partition table.

## Safety — read this before changing `>0502`

**Never use MEMDSK (`>0000`) on a live system disk.** It begins writing at
LBA 0, including the boot/system region. ERASE (`>0005`), WRLBA (`>0003`)
and VERIFY (`>0009`) also overwrite disk contents; VERIFY is **not** a
read-only integrity check. ERASE advances the `LBA_LO` word as it runs.
INIT resets/reconfigures the drive and may interrupt active I/O.

For a non-destructive first inspection, start with the default STATUS
command (`>0001`) or HELP (`>0007`). Confirm the target drive and retain
a backup before attempting any write operation.

