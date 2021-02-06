Boot Process
============

When the N64 boots, the program counter is initially set to 0xBFC00000. You'll notice this is a virtual address in the segment KSEG1 that translates to the physical address 0x1FC00000. While the virtual address 0x9FC00000 seemingly also could be used, this is in KSEG0 which is a cached segment. Because this is the very first code the n64 executes on boot, the caches will not be initialized yet, and this will not work on a real console!

The code at this address is what's called the PIF ROM. This is code baked into the console, and it is used to initialize the hardware and boot the program on the cartridge.

Emulators can execute this code, but the effects of it are so simple it can make sense to simply simulate its effects instead.

Simulating the PIF ROM
----------------------

The PIF ROM can be fairly tricky for young emulators to run, and its effects are not complicated. If you wish to simulate it, know that it has the following side effects on the console:

Set four GPRs to initial values:

+-----------------+---------------+--------------------+
| Register Number | Register Name | Set to value       |
+=================+===============+====================+
| 11              | t3            | 0xFFFFFFFFA4000040 |
+-----------------+---------------+--------------------+
| 20              | s4            | 0x0000000000000001 |
+-----------------+---------------+--------------------+
| 22              | s6            | 0x000000000000003F |
+-----------------+---------------+--------------------+
| 29              | sp            | 0xFFFFFFFFA4001FF0 |
+-----------------+---------------+--------------------+

All other registers are left at zero.

Set some CP0 registers to initial values:

+-----------------+---------------+--------------+
| Register Number | Register Name | Set to value |
+=================+===============+==============+
| 1               | Random        | 0x0000001F   |
+-----------------+---------------+--------------+
| 12              | Status        | 0x70400004   |
+-----------------+---------------+--------------+
| 15              | PRId          | 0x00000B00   |
+-----------------+---------------+--------------+
| 16              | Config        | 0x0006E463   |
+-----------------+---------------+--------------+

All other registers are left at zero.

The value 0x01010101 is then written to physical memory address 0x04300004 (through 0xA4300004 most likely, but that doesn't matter for our purposes here)

The first 0x1000 bytes from the cartridge are then copied to SP DMEM. This is implemented as a copy of 0x1000 bytes from 0xB0000000 to 0xA4000000.

The program counter is then set to 0xA4000040. Note that this skips the first 0x40 bytes of the ROM, as this is where the header is stored. Also note that execution begins with the CPU executing out of SP DMEM.

The ROM now begins to execute! In practice, this is the Bootcode. A reverse-engineering and analysis of this bootcode can be found `Here <https://www.retroreversing.com/n64bootcode>`_.
