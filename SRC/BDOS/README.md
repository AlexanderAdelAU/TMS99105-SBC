# BDOS 6.1 — TMS99105 SBC filesystem architecture

## Overview
This source directory contains BDOS61.A99 and the BDTEST62.A99 regression test for the TMS99105 Single Board Computer (SBC V4). The filesystem architecture below was originally documented for BDOS 6.0; consult BDOS61.A99 for the current implementation. Engineered to maximize the capabilities of the TMS99105 while maintaining legacy compatibility, this OS iteration introduces a massive 16.77 MB contiguous disk geometry and a revolutionary "Folder Alias" compartmentalization system. It achieves isolated directory structures without breaking CP/M-style flat-file application dependencies.

## 1. Hardware Architecture & Constraints
The filesystem is explicitly designed around the hardware behaviors of the SBC V4, specifically its paged memory system.

* **Processor:** Texas Instruments TMS99105.
* **Memory Mapper:** Custom Memory Mapper using local 6116 Memory for page storage.
* **Addressing:** The 6116 is addressed via Memory-Mapped I/O at 0x80C0. The OS utilizes direct MOV instructions to program page registers, strictly avoiding LDCR/STCR CRU instructions.
* **XOP Interrupt Boundaries (Critical):** The SBC hardware automatically disables PSEL (Page Select) during an XOP interrupt (such as an XOP 6 BDOS call). Consequently, BDOS executes entirely within unmapped common memory (Segment F). User applications passing paged DMA addresses must ensure data integrity and proper segment alignment across the unmapped BDOS execution boundary.

## 2. Disk Geometry & Scaling
The OS interfaces with an 8-bit IDE drive controller, scaling file limits to the absolute maximum supported by the allocation math.

* **Total Capacity:** 16.77 MB (16,777,216 bytes).
* **IDE Hardware Translation:** 1 IDE LBA = 256 bytes.
* **BDOS Sector Size:** 512 bytes (2 physical IDE LBAs).
* **Block Size:** 4,096 bytes (4 KB).
* **Sectors Per Block:** 8 sectors.
* **System Blocks:** 4,096 total allocatable blocks.
* **Maximum File Size:** 32,704 records (approx 16.74 MB).
* **Maximum Concurrent Files:** 512 directory entries.

## 3. System Allocation & Block Layout
To protect core OS structures from user-data corruption, Blocks 0 through 7 are permanently isolated from standard file allocation routines. User file allocation begins strictly at Block 8.

| Block(s) | LBA Range | Capacity | Architectural Purpose |
| :--- | :--- | :--- | :--- |
| **0** | 0 - 15 | 4 KB | **Boot Sector:** Bootloader and OS initialization routines. |
| **1 - 2** | 16 - 47 | 8 KB | **BAT (Block Allocation Table):** 16-page map tracking all 4,096 blocks. |
| **3 - 6** | 48 - 111 | 16 KB | **Main Directory:** Stores up to 512 File Control Block (FCB) entries. |
| **7** | 112 - 127 | 4 KB | **Folder Alias Table:** 256-slot dictionary for directory isolation. |
| **8 - 4095** | 128 - 65535 | 16.74 MB | **User Storage:** Dynamically allocated file data space. |

## 4. Directory Compartmentalization (Folder Alias System)
Legacy architectures expect a flat directory. To provide modern nested directories without breaking legacy software, BDOS 6.0 implements a silent "Folder Alias" filter.

### How It Works
1. **The Dictionary (Block 7):** Maintains a mapping of up to 255 unique 11-byte folder names to a single 1-byte ID (ID 0 is reserved for Root).
2. **Context Switching:** When a user executes CHDIR, the OS looks up the 11-byte name in Block 7 and loads the corresponding 1-byte ID into the CFLD (Current Folder ID) variable in system RAM.
3. **Background Filtering:** BDOS silently intercepts all file operations. Directory reads (Functions 17/18) will *only* return files stamped with the active CFLD. New file creations (Function 22) are automatically stamped with the active CFLD.

### Shell Interface
The command environment visually tracks this compartmentalization. If CFLD is non-zero, the Shell prepends the parsed directory name in brackets. 

    % MKDIR UTILITIES
    % CHDIR UTILITIES
    [UTILITIES]% MAKFIL COMPILER.COM
    [UTILITIES]% DIR
    COMPILER COM
    [UTILITIES]% CHDIR
    % DIR
    SHELL    SYS  BDOS     SYS

## 5. File Control Block (FCB) Structure
BDOS relies on a 36-byte FCB passed via Register 3 for all file operations. Key offsets track the file's lifecycle and compartmentalization.

| Offset | Field | Description |
| :--- | :--- | :--- |
| **0-11** | NAM/FTY | 11-byte filename and extension. |
| **12-15** | FSB/FSZ | First Sector Block (0-4095) and total File Size. |
| **18** | FLD_ID | The 1-byte Folder ID for directory isolation. |
| **24-27** | CBN/CRN | Current Block Number and Current Record Number. |
| **28-31** | RELB/RELR | Random Access Block and Record pointers. |

## 6. BDOS 6.0 API / Dispatch Table
File operations are executed via XOP 6. The function code is passed in Register 2 (R2), and the target File Control Block (FCB) pointer is passed in Register 3 (R3).

| Code | Mnemonic | Description |
| :--- | :--- | :--- |
| **1** | CIN | Console Input (Wait for character). |
| **2** | COUT | Console Output (Print character). |
| **15** | OPEN | Open File: Retrieves file size and First Sector Block. |
| **16** | CLOSE | Close File: Flushes buffers and commits final file size. |
| **17** | SEARCH1 | Search Directory: Returns first FCB matching filename and active Folder ID. |
| **19** | ERAFIL | Erase File: Deletes FCB and reclaims all associated BAT blocks. |
| **20** | RDSEQ | Read Sequential: Reads 512 bytes and auto-advances the Current Record Number. |
| **21** | WRSEQ | Write Sequential: Writes 512 bytes, automatically allocating new 4KB blocks as needed. |
| **22** | MAKFIL | Create File: Initializes FCB and stamps it with the active CFLD. |
| **26** | SETDMA | Set DMA: Defines memory target for subsequent disk read/write transfers. |
| **33** | RDRND | Read Random: Reads 512 bytes using absolute 16-bit addressing. |
| **34** | WRRND | Write Random: Writes 512 bytes, handling EOF appends, overwrites, and block bridging. |
| **35** | GETSIZ | Get Size: Returns precise file size in 512-byte records in the random record fields. |
| **36** | SETREL | Set Relative: Copies the Sequential CRN into the Random Record pointer. |
| **41** | MKDIR | Make Directory: Registers a new 11-byte folder string into Block 7. |
| **42** | CHDIR | Change Directory: Validates folder string and updates the CFLD RAM variable. |

## 7. Current source files

- **BDOS61.A99:** The operating-system kernel, file system and BDOS API.
- **BDTEST62.A99:** Filesystem regression and stress-test program; not part
  of the SYSGEN OS payload.
- **DSKINIT62.A99:** The disk initialiser, maintained separately in
  [`SRC/DSKINIT`](../DSKINIT/README.md).

Build the complete OS volume with `MAKE_SYSTEM.bat` from the repository root.
