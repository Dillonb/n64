MIPS Interface
==============

0x0430000 - MI_MODE_REG (Read / Write)
--------------------------------------

Sets and retrieves some values. I am uncertain of what they are used for.

More importantly, this register allows the game to lower the DP interrupt.

Writes
^^^^^^

+-----+-----------------------------------------------------------+
| Bit | Explanation                                               |
+=====+===========================================================+
| 0-6 | Sets init length (??)                                     |
+-----+-----------------------------------------------------------+
| 7   | Clear init mode (??)                                      |
+-----+-----------------------------------------------------------+
| 8   | Set init mode (??)                                        |
+-----+-----------------------------------------------------------+
| 9   | Clear ebus test mode (??)                                 |
+-----+-----------------------------------------------------------+
| 10  | Set ebus test mode (??)                                   |
+-----+-----------------------------------------------------------+
| 11  | Lower DP Interrupt: Sets the bit in MI_INTR_REG to low    |
+-----+-----------------------------------------------------------+
| 12  | Clear RDRAM reg mode (??)                                 |
+-----+-----------------------------------------------------------+
| 13  | Set RDRAM reg mode (??)                                   |
+-----+-----------------------------------------------------------+

Writes with "set" bits high will set the bits in the actual register to high. Writes with "set" bits low will have no effect.

Writes with "clear" bits high will clear the bits in the actual register. Writes with "clear" bits low will have no effect.

If both the "set" and "clear" bits are high, the value is set (?? I think ??)

Reads
^^^^^

+-----+-------------------------------------------------------+
| Bit | Explanation                                           |
+=====+=======================================================+
| 0-6 | Gets init length - returns the value written above    |
+-----+-------------------------------------------------------+
|  7  | Gets init mode - returns the value written above      |
+-----+-------------------------------------------------------+
|  8  | Gets ebus test mode - returns the value written above |
+-----+-------------------------------------------------------+
|  9  | Gets RDRAM reg mode - returns the value written above |
+-----+-------------------------------------------------------+

0x0430004 - MI_VERSION_REG (Read only)
--------------------------------------

+-------+--------------+
| Bit   | Explanation  |
+=======+==============+
| 0-7   | IO (??)      |
+-------+--------------+
| 8-15  | RAC (??)     |
+-------+--------------+
| 16-23 | RDP (??)     |
+-------+--------------+
| 24-31 | RSP (??)     |
+-------+--------------+

This register can be mocked as returning 0x01010101 always (returning 0x01 for each field)

0x0430008 - MI_INTR_REG (Read only)
-----------------------------------

Bits in this register are raised and lowered as interrupts are raised and lowered by other parts of the system.

+-----+----------------------------------------------------------------------------------------------------------------------------+
| Bit | Explanation                                                                                                                |
+=====+============================================================================================================================+
| 0   | SP Interrupt - Set by the RSP when requested by a write to the SP status register, and optionally when the RSP halts.      |
+-----+----------------------------------------------------------------------------------------------------------------------------+
| 1   | SI Interrupt - Set by the serial interface, when SI DMAs to/from PIF RAM finish.                                           |
+-----+----------------------------------------------------------------------------------------------------------------------------+
| 2   | AI Interrupt - Set by the audio interface, when there are no more samples remaining in an audio stream                     |
+-----+----------------------------------------------------------------------------------------------------------------------------+
| 3   | VI Interrupt - Set by the video interface, when V_CURRENT == V_INTR. Allows an interrupt to be raised on a given scanline. |
+-----+----------------------------------------------------------------------------------------------------------------------------+
| 4   | PI Interrupt - Set by the peripheral interface, when a PI DMA between the cartridge and RDRAM finishes.                    |
+-----+----------------------------------------------------------------------------------------------------------------------------+
| 5   | DP Interrupt - Set by the RDP, when a full sync completes.                                                                 |
+-----+----------------------------------------------------------------------------------------------------------------------------+

0x043000C - MI_INTR_MASK_REG (Read / Write)
-------------------------------------------

This register sets up a mask. If (MI_INTR_REG & MI_INTR_MASK_REG) != 0, then a MIPS interrupt is raised.

Writes
^^^^^^

+-----+---------------+
| Bit | Explanation   |
+=====+===============+
| 0   | Clear SP Mask |
+-----+---------------+
| 1   | Set SP Mask   |
+-----+---------------+
| 2   | Clear SI Mask |
+-----+---------------+
| 3   | Set SI Mask   |
+-----+---------------+
| 4   | Clear AI Mask |
+-----+---------------+
| 5   | Set AI Mask   |
+-----+---------------+
| 6   | Clear VI Mask |
+-----+---------------+
| 7   | Set VI Mask   |
+-----+---------------+
| 8   | Clear PI Mask |
+-----+---------------+
| 9   | Set PI Mask   |
+-----+---------------+
| 10  | Clear DP Mask |
+-----+---------------+
| 11  | Set DP Mask   |
+-----+---------------+

See MI_MODE_REG for an explanation on set/clear bits.

Reads
^^^^^

+-----+---------------+
| Bit | Explanation   |
+=====+===============+
| 0   | SP Mask       |
+-----+---------------+
| 1   | SI Mask       |
+-----+---------------+
| 2   | AI Mask       |
+-----+---------------+
| 3   | VI Mask       |
+-----+---------------+
| 4   | PI Mask       |
+-----+---------------+
| 5   | DP Mask       |
+-----+---------------+

Video Interface
===============
TODO

Audio Interface
===============
TODO

Peripheral Interface
====================
TODO

RDRAM Interface
===============
TODO

Serial Interface
================
TODO
