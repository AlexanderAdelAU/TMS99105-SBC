# TMS9900-DISC MONITOR

The ROM Memory Resident Monitor is responsible for providing the core IO and DISC interfaces required to the Single Board Computer.   
For example all the XOP Defintions are defined, such as CALL, PUSH, POP, RET etc as well as all the interfaces routines required
to interface to a Western Digital type Floppy Disc Controller (FDC 1797), and a IDE/SATA interface that will support the latest Solid State Drives (SSD) (version 3).   The Disc IO provides a basic BIOS but relies on a CP/M type DOS system to be fully functional.   Note that for versions 4.0 and greater:
- The Floppy Disc Interfaces have been removed as they are replaced by the IDE, and are therefore redundant.
- Memory segmentation is supported.  For an example see the TMS_99105A_SBC repository.

## The Monitor provides the following commands: (Q,O,[space],G,R,W,U,V) ##

**Q(Quick Boot)**  This will boot the OS from the disc in drive 0
~~~
<TMS9900 DISC MONITOR V2.1>
>Q--Booting....

Shell Version 2.0
Release date 2 Oct 19
%
~~~
**U(Upload)**   This command will upload an Intel hex file into the TMS9900 SBC memory space.  This is how the system files are created etc.  Once the **U** command has been issued, go to your terminal and select the Hex file that you want to upload.  The Upload command will sit forever waiting for the **:** colon to begin.  Once received it will then handshake to upload the full file.  For example, here the dots indicate each 16 byte block of data (as per the intel format):

~~~
<TMS9900 DISC MONITOR V2.1>
>U......................................................................................
~~~
Once uploaded, the loaded file can be exectued using the **<hex address>G(Go)>** command.

**V (Upload Segment Hex File)**.  This command will allow you to upload a segment hex file.  This is a modified Intel hex file that contains an additional byte that is inserted before the address field to indicate which memory segment to insert the data into.  These files are identified in assembler source code with the use of the SEG psuedo op.  For example the following will inster the hex 0x01 into the H99 file just before the address field:
```
	SEG  <0,1> 	; Either 0 or 1
	e.g.
	SEG 1
	AORG	500H
```
During upload the segment V command will insert a plus sign in the response to indicate that the file is inserting into the requested memory segment.
~~~
<TMS9900 DISC MONITOR V43>
>V.+.+.+.+.+.+.+.+.+.+.+.+.+.+.
~~~
**< HexAddress >O** Open.  This command Will open or list of 8 rows displaying the memory contents begining at the HexAddress.  Press Space to show the next 8 rows etc.
~~~
>C300O
    C300  0460 C31A 0318 1319 1A20 0008 0D0A 07FF
    C310  3A2E 2E2E 5F25 242A 2B00 02E0 CFA0 020A
    C320  C2FE D820 C30C CC3A 0200 CC3A C800 00A0
    C330  04E0 00A2 0200 CCA4 C800 00A4 0200 0460
    C340  0201 C4CE C800 0080 C801 0082 0200 0460
    C350  0201 D100 C800 0084 C801 0086 0200 0100
    C360  C800 00A6 2FA0 CEAA C020 CC2E 04E0 CC2E
    C370  C000 1616 04C1 D060 CC3A 9801 C30C 1310

~~~
**< HexAddress >Space**.  Pressing space bar Will open the memory location for editing.  Space will continue the editing to the next memory address

**< HexAddress >G** Go.  This command will go to the memory specified memory location and begin running the code at that location.  So, you can use the Upload command to load the Shell, DOS etc and then use the Go command to run the disc initialisation or (DISKINIT) programme.

**< HexAddress >W** Workspace.  This will enter the hexaddress into the workspace register and define your workspace.  W by itself will print the 
 value of current workspace register.

 ```
>230W
>W
     WP = 0230

```

**< HexAddress >R** Registers.  This command will print out the workspace registers located at the hexaddress, or will use the current workspace value.
For example:
~~~
>400R
     WP=0400
    R0  = DCA2 R1  = A8F9
    R2  = ED20 R3  = F66A
    R4  = ED30 R5  = F638
    R6  = ED40 R7  = F66E
    R8  = E8BD R9  = 26BD
    R10 = C900 R11 = 703B
    R12 = B235 R13 = 783D
    R14 = 1185 R15 = E4E1
~~~

These basic commands are generally all that is required of a basic Monitor as the majority of all the other tasks can be use normal programmes and the Operating System.






