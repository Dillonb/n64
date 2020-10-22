Overview
========


The Nintendo 64 is an early 3D console with some interesting quirks. Let's get right into it.

The system is often thought of as having two main components - the CPU, and the Reality Coprocessor (RCP), with the RCP again being divided into two components, the Reality Signal Processor (RSP) and the Reality Display Processor (RDP.)

The system has 4MiB of RDRAM, expandable with an add-on (Expansion Pak) to 8MiB. The system is designed in such a way that all 3 processors (CPU, RSP, RDP) can access the same memory. Thus, this RAM acts as both normal system RAM and VRAM at the same time.

Interestingly, each byte in RDRAM is actually 9 bits. In other computer systems with RDRAM, this 9th bit is normally used as a parity bit for data integrity checking, but it has been repurposed in the N64 as extra storage for the RDP. The RDP uses it for depth buffering and anti-aliasing, usually.

The 9th bits are usually implemented in emulators as an entirely separate structure from the main RDRAM array.

CPU Overview
------------

The CPU is a fairly standard 64 bit MIPS r4300i chip.

It has 32 *64-bit* registers. The first register, r0 or $zero, is hardwired to a value of 0, at all times. There are also special-purpose registers for things like multiplication.

It has a 32 bit program counter and uses 32 bit addresses to access memory. It can load and store 8, 16, 32, and 64 bit values. All memory accesses must be aligned, and load/stores with unaligned addresses will throw exceptions.

All memory accesses, including instruction fetches, use virtual memory. There are segments of the address space that use fixed translation, and segments that are configurable.

MIPS has the concept of "coprocessors" built directly into the instruction set. Each one is numbered, and there are special opcodes for interacting with them, including MTCx/MFCx (move to/from coprocessor x), which are used to move data between the main CPU registers and the coprocessor's registers.

Coprocessor zero, or CP0, is the "system control coprocessor." It is through CP0 that virtual memory is set up, exceptions and interrupts are controlled, the CPU status is accessed, among other things. There are 32 32-bit registers, most of which have special uses.

Coprocessor one, or CP1, is a floating-point unit, or FPU. It supports a variety of floating point operations on its 32 floating-point registers.


Reality Coprocessor Overview
----------------------------

As mentioned above, the RCP is actually composed of two separate components, the RSP and the RDP.

Reality Signal Processor Overview
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The RSP is a secondary CPU mostly used for 3D and audio calculations. While it runs at 2/3rds the speed of the main CPU, it has powerful *Single Instruction Multiple Data* (SIMD) instructions that make it extremely suited for the type of math required by 3D graphics. These instructions perform the same operation on multiple values at once.

It is effectively a stripped-down version of the MIPS chip used for the main CPU, missing some instructions and running at a slightly slower speed.

It has 32 *32-bit* registers. Note that this is different than the main CPU, which has 64-bit registers! As in the CPU, the first register, r0 or $zero, is hardwired to 0 at all times.

The RSP has no CP0 in the same way the CPU does. Instead, the CP0 registers are used to communicate with the DMA engine, RDP, and various other things. It also does not have a floating point unit available on CP1.

It, however, does have a Vector Unit (VU) available as CP2. The VU has quite a few registers which are discussed in the RSP section below. Sometimes, the normal operations of this CPU are referred to as the Scalar Unit (SU) to differentiate them from the vector unit.

All memory accesses use a physical address, there is no virtual memory involved here.

Unlike the CPU, the RSP is capable of reading and writing unaligned values. Not only will these accesses not throw exceptions, they'll work perfectly! Again, note that memory accesses can only be to DMEM.

SP IMEM is only 0x1000 bytes in size and the bottom 12 bits of the program counter are used to address it. Because of this, the program counter can be thought of as being a simple 12 bit value.


Reality Display Processor Overview
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The RDP is a rasterizer used to display images on-screen.

.. toctree::
   :maxdepth: 2

   memory_map
   cpu
