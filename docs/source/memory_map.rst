Memory Map
==========

Virtual Memory Map
-------------------

**ALL** memory accesses made by the main CPU, both instruction fetches and through load/store instructions, use virtual addresses. These addresses are translated to physical addresses by the Memory Management Unit (MMU) before they are actually used to access hardware.

+-----------------------+-------+--------------------------------------------+
| Address range         | Name  | Description                                |
+=======================+=======+============================================+
| 0x00000000-0x7FFFFFFF | KUSEG | User segment. TLB mapped                   |
+-----------------------+-------+--------------------------------------------+
| 0x80000000-0x9FFFFFFF | KSEG0 | Kernel segment 0. Direct mapped, cached.   |
+-----------------------+-------+--------------------------------------------+
| 0xA0000000-0xBFFFFFFF | KSEG1 | Kernel segment 1. Direct mapped, no cache. |
+-----------------------+-------+--------------------------------------------+
| 0xC0000000-0xDFFFFFFF | KSSEG | Kernel supervisor segment. TLB mapped.     |
+-----------------------+-------+--------------------------------------------+
| 0xE0000000-0xFFFFFFFF | KSEG3 | Kernel segment 3. TLB mapped.              |
+-----------------------+-------+--------------------------------------------+

Note that the cacheing mentioned above is not critical for emulation.

Virtual-to-Physical Address Translation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
For the direct mapped segments (KSEG0 and KSEG1), the translation is easy. Simply subtract the start of the segment the address is in from the address itself.

For example, both virtual address 0x80001000 (in KSEG0) and virtual address 0xA0001000 would translate to the physical address 0x00001000.

For TLB-mapped segments, things are a bit more complicated. See the :ref:`TLB section<TLB>` for more details.

Physical Memory Map
-------------------
TODO

.. toctree::
   :caption: More info
   :maxdepth: 2

   tlb
