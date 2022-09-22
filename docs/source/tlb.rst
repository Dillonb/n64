TLB
===

The TLB (transfer lookaside buffer) converts virtual addresses into physical addresses. The TLB is a fully associative cache with 32 entries, entries are mapped with odd and even numbers in pairs. When a virtual address is given, each entry in the TLB checks the 32 entries for whether they coincide with the address in the "address space identification" (ASID) area in the "Entry Hi" register. If it does, a "hit" occurs, and a physical address is generated in the TLB as well as an offset. If a miss occurs, then an exception occurs and the TLB entry is written by the software to a page table in memory, either to the TLB entry over the selected index register, or it writes to a random entry indicated by the random register. If 2 or more are hits, the TLB isn't correctly executed, and the TLB shutdown (TS) bit of the status register is set to 1, and the TLB cannot be used.

Format of a virtual address: [G|ASID|VPN | OFFSET]

Converting a virtual address to a physical address begins by comparing the virtual address from the VR4300 MMU with the virtual addresses in the TLB; there is a match when the virtual page number (VPN) of the address is the same as the VPN field of the entry, and either:

A. The Global (G) bit of the TLB entry is set, or
B. The ASID field of the virtual address is the same as the ASID field of the TLB entry.

This match is referred to as a TLB "hit". If there is no match, a "TLB Miss" exception occurs in the CPU and software is allowed to reference a page table of virtual/physical addresses in memory and to write its contents to the TLB. If there is a virtual address match in the TLB, the physical address is output from the TLB and concatenated with the offset, which represents an address within the page frame space. The offset does not pass through the TLB. The lower bits of the virtual address are output as is. 

The TLB page coherency attribute (C) bits specify whether references to the page should be cached or not, and, if cached, then selects between several coherency attributes. The table below shows the coherency attributes selected by the various C bits.

TLB Page Coherency (C) Bit Values

1 - Reserved
2 - Reserved
3 - Uncached
4 - Cacheable noncoherent (noncoherent)
5 - Cacheable coherent exclusive (exclusive)
6 - Cacheable coherent exclusive on write (shareable)
7 - Cacheable coherent update on write (update)
8 - Reserved