# Transparent memory mapping on the TMS99105 SBC

The CPU can address 64K. The board has 1MB. Rather than intercept the address
bus, this design leaves it completely alone and has a GAL contribute four
*extra* address lines alongside it — so a program uses far more memory than it
can address, at no cost in cycles and without knowing anything about it.

FileEdit is the worked example: an editor whose code is roughly twice the size
of the space it occupies.

---

## The hardware: the address bus is never touched

```mermaid
flowchart LR
    CPU["TMS99105"]
    CPU -->|"A0-A14<br/>the full 64K, unaltered"| RAM["HM628512 pair<br/>SRAM A4-A18"]
    CPU -->|"A0-A3<br/>which 4K segment"| MUX["74LS157<br/>address / map-write"]
    MUX --> M6116["6116<br/>16 segment entries"]
    M6116 -->|"D4-D7"| GAL["GAL 22V10"]
    GAL -->|"SA0-SA3<br/>which page"| RAM
    GAL -->|"ROM_SEL RAM_SEL WAIT"| RAM
```

The CPU's `A0-A14` go straight to the SRAMs and address the whole 64K exactly
as they would with no mapper present. Nothing is intercepted, substituted or
delayed.

In parallel, the top four address bits index a 6116 holding one entry per 4K
segment. Its upper nibble reaches the GAL, which emits **SA0-SA3** — four
*additional* address lines. The SRAM address is therefore 19 bits wide where
the CPU only drove 15: the address space is **extended**, not rewritten.

| SRAM pins | Driven by |
| --- | --- |
| `A0-A3` | `SA0-SA3` from the GAL — the page |
| `A4-A18` | `A0-A14` from the CPU — the 64K address |

A GAL term decides which segments are mapped at all:

```
IS_PAGED = (A0 & !A1) # (A1 & !A2) # (A2 & !A0) # (A3 & !A0)
```

| Segments | Behaviour |
| --- | --- |
| 0 (`>0000-0FFF`) | **Common** — always page 0 |
| 1-D (`>1000-DFFF`) | **Paged** — whatever the 6116 says |
| E, F (`>E000-FFFF`) | **Common** — BDOS and ROM, always present |

Common segments matter more than they look. Code that changes the map cannot
live in a segment it is changing, so the gates that switch pages sit in segment
0, and the BDOS stays reachable whatever the mapper is doing.

### Free of charge, in the shadow of the address latch

```mermaid
flowchart LR
    T1["ALATCH falls<br/>address valid"] --> T2["MAP_SEL low<br/>6116 read"]
    T2 --> T3["SA0-SA3 valid"]
    T3 --> T4["memory cycle<br/>no wait states"]
```

The lookup overlaps the address-latch phase of the same cycle. `MAP_SEL` goes
low at mid-ALATCH and stays low 218.75 ns — 3.5 T at 16 MHz — which is settled
well before the data is needed. The mapper costs no CPU time at all.

### One 6116, two jobs

The same 6116 has to be written by the CPU when the map changes, and read by
the hardware on every cycle. A 74LS157 switches its address inputs: `A0-A3`
during normal operation, `A11-A14` when `MAP_REG` selects the map for writing.
Only the upper nibble of each entry is used, so sixteen bytes describe the
entire address space.

---

## FileEdit's address space

```mermaid
flowchart TB
    subgraph CPU["What the CPU sees — 64K"]
        direction TB
        A["&gt;C000  shell / monitor"]
        B["&gt;8000-BFFF  WINDOW A<br/>one overlay at a time"]
        C["&gt;1000-7FFF  resident — 28K<br/>editor core, buffer, terminal, file I/O"]
        D["&gt;0000-0FFF  common — vectors, gates, sector buffer"]
    end
```

The resident 28K is always there: the edit buffer, the menu, terminal handling,
the file layer, and the overlay manager itself. Only window A changes.

---

## Five overlays, one window

```mermaid
flowchart LR
    subgraph PHYS["Physical pages"]
        P2["page 2<br/>OVL_WORD"]
        P3["page 3<br/>OVL_CHAR"]
        P4["page 4<br/>OVL_RPT"]
        P5["page 5<br/>OVL_VIEW"]
        P6["page 6<br/>OVL_EDIT"]
    end
    WIN["segment 8<br/>&gt;8000-8FFF"]
    P2 -.-> WIN
    P3 -.-> WIN
    P4 -.-> WIN
    P5 -.-> WIN
    P6 -.-> WIN
```

Every overlay is compiled to run at `>8000`. Only one is mapped at a time, and
the choice is a single CRU write.

| ID | Overlay | Entry | Job |
| --- | --- | --- | --- |
| 1 | OVL_WORD | `>80DC` | word scan |
| 2 | OVL_CHAR | `>8000` | character scan |
| 3 | OVL_RPT | `>8000` | statistics report |
| 4 | OVL_VIEW | `>83EC` | full-screen viewer |
| 5 | OVL_EDIT | `>8BF0` | the editor itself |

The five together are about 11K of code sharing 4K of address space. The
manager also supports up to five `(segment, page)` pairs per overlay, so one
overlay can span a contiguous 16K window when it outgrows a single page —
FileEdit doesn't currently need that.

---

## Calling into an overlay

```mermaid
sequenceDiagram
    participant App as Resident code
    participant Mgr as OVLMGR
    participant CRU as 6116 via CRU
    participant Ovl as Window A at 8000

    App->>Mgr: R1 = overlay ID
    Note over Mgr: look up (segment,page)<br/>in OVL_TABLE
    alt page already mapped
        Mgr-->>App: return — nothing to do
    else different page
        Mgr->>CRU: PSEL_DIS
        Mgr->>CRU: LDCR page, MAP_WIN_BASE + segment*2
        Mgr->>CRU: PSEL_EN
        Mgr-->>App: return
    end
    App->>Ovl: CALL @O_EDIT
```

From the caller it is two lines:

```
    LI   R1,OVL_EDIT
    CALL @OVLMGR
    CALL @O_EDIT
```

`OVLMGR` keeps a `CURRENT_PAGE` entry per segment and returns immediately if
the page it wants is already there, so a repeated call into the same overlay
costs a compare, not a remap.

---

## Why this is worth the trouble

- **The compiler never knows.** Every overlay is ordinary C compiled to run at
  `>8000`. No far pointers, no banking annotations, no special calling
  convention.
- **Swapping is a CRU write.** No copying, no disk access. The pages are loaded
  once when the program starts and simply mapped in and out.
- **Direct calls within an overlay.** Pages of one overlay are co-resident, so
  code inside it calls itself normally. Only calls *between* different overlays
  are forbidden — and those go through the manager.
- **The address is stable.** `A4-A15` pass through untouched, so an address
  inside a page means the same offset whichever page is mapped. That is what
  makes the relocation trivial.

The cost is one rule: two overlays are never present at once, so nothing in one
may call directly into another. The build enforces the layout — DREL generates
`OVLADDR.INC` and `OVLTABLE.INC` from the linked objects, so the table and the
entry points cannot drift apart by hand.

---

## Where each piece lives

| File | Role |
| --- | --- |
| `OVLMGR.A99` | the manager — table walk and the CRU write |
| `OVLAPI.A99` | fixed entry slots the resident code calls |
| `OVLADDR.INC` | generated: overlay ID → entry address |
| `OVLTABLE.INC` | generated: overlay ID → `(segment, page)` pairs |
| `src/overlays/*.C` | the overlays themselves, plain C |
| `src/resident/*.C` | everything that must always be present |
