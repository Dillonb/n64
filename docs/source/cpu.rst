CPU (detailed)
==============


Registers
---------

As noted in the CPU overview, r0 is **hardwired to a value of zero**, and all registers are 64 bits.

+-----------------+---------------+
| Register Number | Register Name |
+=================+===============+
| 0               | zero          |
+-----------------+---------------+
| 1               | at            |
+-----------------+---------------+
| 2               | v0            |
+-----------------+---------------+
| 3               | v1            |
+-----------------+---------------+
| 4               | a0            |
+-----------------+---------------+
| 5               | a1            |
+-----------------+---------------+
| 6               | a2            |
+-----------------+---------------+
| 7               | a3            |
+-----------------+---------------+
| 8               | t0            |
+-----------------+---------------+
| 9               | t1            |
+-----------------+---------------+
| 10              | t2            |
+-----------------+---------------+
| 11              | t3            |
+-----------------+---------------+
| 12              | t4            |
+-----------------+---------------+
| 13              | t5            |
+-----------------+---------------+
| 14              | t6            |
+-----------------+---------------+
| 15              | t7            |
+-----------------+---------------+
| 16              | s0            |
+-----------------+---------------+
| 17              | s1            |
+-----------------+---------------+
| 18              | s2            |
+-----------------+---------------+
| 19              | s3            |
+-----------------+---------------+
| 20              | s4            |
+-----------------+---------------+
| 21              | s5            |
+-----------------+---------------+
| 22              | s6            |
+-----------------+---------------+
| 23              | s7            |
+-----------------+---------------+
| 24              | t8            |
+-----------------+---------------+
| 25              | t9            |
+-----------------+---------------+
| 26              | k0            |
+-----------------+---------------+
| 27              | k1            |
+-----------------+---------------+
| 28              | gp            |
+-----------------+---------------+
| 29              | sp            |
+-----------------+---------------+
| 30              | s8            |
+-----------------+---------------+
| 31              | ra            |
+-----------------+---------------+

While these all have special names, for the purposes of emulation they all operate identically (except for r0 and r31)

As I've mentioned many times by now, r0 is hardwired to 0, and r31, or $ra, is used by the "branch/jump and link" instructions to hold the return address.

The various other names are useful for assembly programmers who need to know what registers are conventionally used for what purposes.

CP0 Registers
-------------

+-----------------+---------------+----------------------------------+
| Register Number | Register Name | Size                             |
+=================+===============+==================================+
| 0               | Index         | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 1               | Random        | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 2               | EntryLo0      | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 3               | EntryLo1      | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 4               | Context       | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 5               | PageMask      | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 6               | Wired         | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 7               | 7             | ??                               |
+-----------------+---------------+----------------------------------+
| 8               | BadVAddr      | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 9               | Count         | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 10              | EntryHi       | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 11              | Compare       | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 12              | Status        | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 13              | Cause         | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 14              | EPC           | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 15              | PRId          | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 16              | Config        | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 17              | LLAddr        | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 18              | WatchLo       | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 19              | WatchHi       | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 20              | XContext      | 64 bits                          |
+-----------------+---------------+----------------------------------+
| 21              | 21            | ??                               |
+-----------------+---------------+----------------------------------+
| 22              | 22            | ??                               |
+-----------------+---------------+----------------------------------+
| 23              | 23            | ??                               |
+-----------------+---------------+----------------------------------+
| 24              | 24            | ??                               |
+-----------------+---------------+----------------------------------+
| 25              | 25            | ??                               |
+-----------------+---------------+----------------------------------+
| 26              | Parity Error  | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 27              | Cache Error   | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 28              | TagLo         | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 29              | TagHi         | 32 bits                          |
+-----------------+---------------+----------------------------------+
| 30              | ErrorEPC      | 32 or 64 bits, depending on mode |
+-----------------+---------------+----------------------------------+
| 31              | 31            | ??                               |
+-----------------+---------------+----------------------------------+

CP0 TLB Registers
^^^^^^^^^^^^^^^^^

These registers are used to query and control the TLB. Please see the TLB section for more information.

* Index
* EntryLo0
* EntryLo1
* EntryHi
* PageMask
* Context

CP0 Random Number Registers
^^^^^^^^^^^^^^^^^^^^^^^^^^^

These registers are used to generate random values.

The Random register is read-only. The high 26 bits are unused, leaving the low 6 bits to represent a random value. This value can be read and used by software, but is mainly meant to be used by the TLBWR (TLB Write Random) instruction.

On a real CPU, the value is decremented every instruction. When the value of Random is <= the value of Wired, it is reset to 0x1F (31)

It should be fine for emulation purposes to generate a random value in the range of Wired <= Value <= 31 every time Random is read, as checking and decrementing Random every single instruction will be expensive.

* Random

Holds a random value between the value of Wired and 0x1F (31)

* Wired

Provides the lower bound for the random value held in Random.

CP0 Timing Registers
^^^^^^^^^^^^^^^^^^^^

Since the N64 has no timers, these registers are the only way the system can tell how much time has passed.

* Count

This value is incremented every other cycle, and compared to the value in Compare. As noted below, fire an interrupt when Count == Compare.

The easiest way to emulate this would be to store count as a 64 bit integer, increment it once per cycle, and shift it to the right by one when read or compared.

* Compare

Fire an interrupt when Count equals this value. This interrupt sets the ip7 bit in Cause to 1.

Writes to this register clear said interrupt, and sets the ip7 bit in Cause to 0.

CP0 Cache Registers
^^^^^^^^^^^^^^^^^^^

These registers are used for the cache, which is not documented here yet.

* TagLo
* TagHi

CP0 Exception/Interrupt Registers
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These registers are used for exceptions and interrupts.

* BadVAddr
* Cause
* EPC
* ErrorEPC
* WatchLo
* WatchHi
* XContext
* Parity Error

  The N64 does not generate a parity error, so this register is never written to by hardware.

* Cache Error

  The N64 does not generate a cache error, so this register is never written to by hardware.

CP0 Other Registers
^^^^^^^^^^^^^^^^^^^

These registers don't fit cleanly into any other category.

* PRId
* Config
* LLAddr
* Status
