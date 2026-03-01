# Intel Hex Merger for SBC

A lightweight, K&R-compliant C utility designed to combine multiple Intel HEX files into a single binary image. This tool is specifically optimized for older Single Board Computers (SBCs) and serial loaders (such as those for the TMS9900) that are sensitive to line endings, checksum structures, and record boundaries.

---

## Features

- **Strict Line Formatting:** Forces `\r` (Carriage Return) line endings, ensuring compatibility with legacy serial buffers.
- **Intelligent Merging:** Strips “poison” No-Op records (`:0000000000`) and redundant EOF markers from intermediate files.
- **Automated EOF Management:** Automatically ensures the End-of-File marker appears only once at the very end of the final output.
- **Portable:** Written in strict K&R C, making it highly portable across various compiler environments.

---

## Compilation

Compile using any standard C compiler (for example, `gcc`):

```bash
gcc -o mergehex merge.c
```

---

## Usage

Example merge command:

```bash
mergehex SYSGEN.H99 BDOS47F.H99 SHELLV47.H99 DSKINIT47.H99

Starting merge process...
Output destination: SYSGEN.H99
  Merging: BDOS47F.H99
  Merging: SHELLV47.H99
  Merging: DSKINIT47.H99
Success: Processed 3 files.
Final combined hex file saved as: SYSGEN.H99
```

This produces a single merged Intel HEX output file suitable for upload to your SBC.

---

## Upload to SBC

On the SBC monitor:

1. Upload the merged HEX image:

    ```
    U ->  Then From host machine select File->SYSGEN.H99 to upload.
    ```

2. Execute the system generator:

    ```
    0500G
    ```

3. Boot the operating system:

    ```
    Q
    ```
