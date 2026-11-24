# SYSGEN: TMS99105 SBC V4 OS Generation Utility

The current source is `SRC/SYSGEN/sysgen.c`. Run `MAKE_SYSTEM.bat` at the
repository root to compile SYSGEN and package the entire fourteen-file
Shell 6.5 volume. The shorter usage example below illustrates the packer
interface, but the master batch file is authoritative for the current file
list and order.

**SYSGEN** is a lightweight, fast, and K&R-compliant C utility designed to package operating system modules for the TMS99105 Single Board Computer (SBC V4). It parses, cleans, and merges multiple Intel Hex (`.H99`) files into a single, unified payload ready for serial transmission and disk initialization.

## 🛠️ How It Works

When developing an OS in modular assembly, the assembler outputs independent Intel Hex files. `sysgen` solves the challenge of moving these disparate modules onto the target hardware's IDE disk while providing the necessary file system metadata to the bootstrap initializer.

1. **Input Parsing & Mapping:** Reads multiple `.H99` files. It supports a unique `TARGET=SOURCE` syntax to map a local source hex file to its final file system name on the SBC.
2. **Sanitization:** Safely handles carriage return (CR) and line feed (LF) line-ending inconsistencies across different platforms, ensuring the output is perfectly formatted for the SBC ROM monitor's serial parser.
3. **Concatenation & Metadata Injection:** Strips redundant End-Of-File (EOF) markers from intermediate files. For mapped files, it injects the target filename (padded to 11 bytes) into the data stream so the disk initializer (`DSKINIT`) knows exactly what to name the file in the directory sector.
4. **Payload Generation:** Outputs a single, clean `PAYLOAD.HEX` file that can be pushed over a serial link.

## 🚀 Usage

Compile the utility using your preferred C compiler.

**Syntax:**
`sysgen <output_file> <boot_initializer> [TARGET_NAME=SOURCE_FILE] ...`

*   **output_file:** The final combined Hex file to be generated.
*   **boot_initializer:** The bare-metal initialization program (e.g., `DSKINIT`). This file is passed without an alias because it executes directly from memory and does not get written to the file system.
*   **TARGET_NAME=SOURCE_FILE:** The OS modules to be written to disk. The `TARGET_NAME` is the final 11-character name (e.g., `SHELL.SYS`) that `DSKINIT` will write to the disk directory. The `SOURCE_FILE` is your local assembly output (e.g., `SHELLV65.H99`).
*   **FOLDER/TARGET_NAME=SOURCE_FILE:** Puts the file in a folder instead of the root (`\` works too). SYSGEN creates each folder the first time it is named, giving the folders IDs 1, 2, ... in that order, and writes the Folder Alias Table (block 7, LBA 112) through the write table, after `DSKINIT` has wiped it. `MAKE_SYSTEM.bat` uses this to put the ten `.SHC` command pages in folder `SHC`, which must therefore be the first folder named.

### Limits

SYSGEN stops with `*** SYSGEN ERROR` (and `MAKE_SYSTEM.bat` reports `BUILD FAILED`) rather than produce a payload that would silently corrupt:

| Limit | Value | Why |
| --- | --- | --- |
| Files | 16 | One 512-byte directory sector holds 16 entries |
| Payload data | `>3000`–`>EFFF` | The monitor's `U` loads the payload flat, below the ROM at `>F000` |
| DSKINIT | must end below `>2000` | SYSGEN's BAT, directory and write table start there |
| Disk blocks | 256 | One BAT sector |

Each build prints how much it used, e.g. `directory 14 of 16 entries, payload data >3000-75FF (limit >F000)`.

### Example

To build a fresh OS payload consisting of the initializer, Shell, BDOS, XMODEM, and Directory utilities:

```bash
sysgen PAYLOAD65.HEX DSKINIT62.H99 SHELL.SYS=SHELLV65.H99 BDOS.SYS=BDOS61.H99 XMODEM.COM=XMODEM59.H99 DIR2.COM=DIR2.H99
```

---

## 💾 Uploading and Bootstrapping the OS

Once you have generated your payload hex file, follow these steps to upload it to the SBC V4 and bootstrap the operating system onto the IDE disk.

### Step 1: Prepare the Target
1. Connect to the SBC V4 via your serial terminal emulator.
2. Ensure the system is sitting at the ROM Monitor prompt (`>`).

### Step 2: Bootstrapping the OS

Use the ROM monitor's native commands to push the payload to memory, execute the initialization routine, and boot the system:

```text
<TMS9900 DISC MONITOR V5.5F>
>U     --> upload PAYLOAD65.HEX
>500G  --> bootstrap OS onto disc
>Q     --> Boot the OS

Shell Version 6.5
TMS99105 SBC V4 - Paged Memory
%DIR2

FILENAME TYPE    SIZE    LOAD
-------- ---- -------    ----
SHELL     SYS      7168  C000
BDOS      SYS      2560  E000
XMODEM    COM      2048  1000
DIR2      COM      1024  0500

FREE CAPACITY:   16332 KB
%
```
