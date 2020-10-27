Boot Process
============

When the N64 boots, the program counter is initially set to 0xBFC00000. You'll notice this is a virtual address in the segment KSEG1 that translates to the physical address 0x1FC00000. While the virtual address 0x9FC00000 seemingly also could be used, this is in KSEG0 which is a cached segment. Because this is the very first code the n64 executes on boot, the caches will not be initialized yet, and this will not work on a real console!

The code at this address is what's called the PIF ROM. This is code baked into the console, and it is used to initialize the hardware and boot the program on the cartridge.

Emulators can execute this code, but the effects of it are so simple it can make sense to simply simulate its effects instead.

If you wish to simulate it, simply perform the following operations:


Set three GPRs to initial values:

+-----------------+---------------+--------------------+
| Register Number | Register Name | Set to value       |
+=================+===============+====================+
| 20              | s4            | 0x0000000000000001 |
+-----------------+---------------+--------------------+
| 22              | s6            | 0x000000000000003F |
+-----------------+---------------+--------------------+
| 29              | sp            | 0x00000000A4001FF0 |
+-----------------+---------------+--------------------+


**TODO: is $sp set to 0x00000000A4001FF0, or 0xFFFFFFFFA4001FF0?** (i.e., is it sign extended?)

All other registers are left at zero.

Set some CP0 registers to initial values:

+-----------------+---------------+--------------+
| Register Number | Register Name | Set to value |
+=================+===============+==============+
+-----------------+---------------+--------------+
| 1               | Random        | 0x0000001F   |
+-----------------+---------------+--------------+
| 12              | Status        | 0x70400004   |
+-----------------+---------------+--------------+
| 15              | PRId          | 0x00000B00   |
+-----------------+---------------+--------------+
| 16              | Config        | 0x0006E463   |
+-----------------+---------------+--------------+

All other registers are left at zero.

The value 0x01010101 is then written to memory address 0x04300004

The first 0x1000 bytes from the cartridge are then copied to the first 0x1000 bytes of RDRAM. This is implemented as a copy of 0x1000 bytes from 0xB0000000 to 0xA4000000.

The program counter is then set to 0xA4000040. Note that this skips the first 0x40 bytes of the ROM, as this is where the header is stored.

