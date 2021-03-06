MIPS Interface
==============

0x04300000 - MI_MODE_REG (Read / Write)
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

0x04300004 - MI_VERSION_REG (Read only)
--------------------------------------

+-------+--------------+
| Bit   | Explanation  |
+=======+==============+
| 0-7   | IO Version   |
+-------+--------------+
| 8-15  | RAC Version  |
+-------+--------------+
| 16-23 | RDP Version  |
+-------+--------------+
| 24-31 | RSP Version  |
+-------+--------------+

This register should return 0x02020102 always.

0x04300008 - MI_INTR_REG (Read only)
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

0x0430000C - MI_INTR_MASK_REG (Read / Write)
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

0x04400000 - VI_STATUS_REG/VI_CONTROL_REG
-----------------------------------------
Can be called the VI_STATUS_REG, or the VI_CONTROL REG, whichever you prefer.

This register describes the format of the framebuffer in RDRAM, as well as enables and disables effects such as gamma, dithering, anti-aliasing, etc.

+-------+----------------------------------------+
| Bit   | Explanation                            |
+-------+----------------------------------------+
| 0-1   | Framebuffer bits-per-pixel (see below) |
+-------+----------------------------------------+
| 2     | Gamma dither enable                    |
+-------+----------------------------------------+
| 3     | Gamma enable                           |
+-------+----------------------------------------+
| 4     | Divot enable                           |
+-------+----------------------------------------+
| 5     | Reserved                               |
+-------+----------------------------------------+
| 6     | Serrate                                |
+-------+----------------------------------------+
| 7     | Reserved                               |
+-------+----------------------------------------+
| 8-9   | Anti-alias mode (see below)            |
+-------+----------------------------------------+
| 10    | Unused                                 |
+-------+----------------------------------------+
| 11    | Reserved                               |
+-------+----------------------------------------+
| 12-15 | Reserved                               |
+-------+----------------------------------------+
| 16-31 | Unused                                 |
+-------+----------------------------------------+

Enum Definitions
^^^^^^^^^^^^^^^^

Framebuffer bits per pixel:
  0. Blank
  1. Reserved
  2. RGBA 5553 "16" bits per pixel (should be able to ignore alpha channel and treat this as RGBA5551)
  3. RGBA 8888 32 bits per pixel

Anti-alias mode:
  0. Anti-alias and resample (always fetch extra lines)
  1. Anti-alias and resample (fetch extra lines if needed)
  2. Resample only (treat as all fully covered)
  3. No anti-aliasing or resampling, no interpolation.


0x04400004 - VI_ORIGIN_REG
--------------------------

Describes where in RDRAM the VI should display the framebuffer from. Bits 0 through 23 are used, bits 24 through 31 are ignored by hardware.

+-------+------------------------------+
| Bit   | Description                  |
+-------+------------------------------+
| 0-23  | RDRAM address of framebuffer |
+-------+------------------------------+
| 24-31 | Unused                       |
+-------+------------------------------+

0x04400008 - VI_WIDTH_REG
-------------------------
TODO

0x0440000C - VI_INTR_REG
------------------------
TODO

0x04400010 - VI_V_CURRENT_REG
-----------------------------
TODO

0x04400014 - VI_BURST_REG
-------------------------
TODO

0x04400018 - VI_V_SYNC_REG
--------------------------
TODO

0x0440001C - VI_H_SYNC_REG
--------------------------
TODO

0x04400020 - VI_LEAP_REG
------------------------
TODO

0x04400024 - VI_H_START_REG
---------------------------
TODO

0x04400028 - VI_V_START_REG
---------------------------
TODO

0x0440002C - VI_V_BURST_REG
---------------------------
TODO

0x04400030 - VI_X_SCALE_REG
---------------------------
TODO

0x04400034 - VI_Y_SCALE_REG
---------------------------
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
