# SYSGEN: TMS99105 SBC V4 OS Generation Utility

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
*   **TARGET_NAME=SOURCE_FILE:** The OS modules to be written to disk. The `TARGET_NAME` is the final 11-character name (e.g., `SHELL.SYS`) that `DSKINIT` will write to the disk directory. The `SOURCE_FILE` is your local assembly output (e.g., `SHELLV63.H99`).

### Example

To build a fresh OS payload consisting of the initializer, Shell, BDOS, XMODEM, and Directory utilities:

```bash
sysgen PAYLOAD65.HEX DSKINIT62.H99 SHELL.SYS=SHELLV63.H99 BDOS.SYS=BDOS61.H99 XMODEM.COM=XMODEM58.H99 DIR2.COM=DIR2.H99
