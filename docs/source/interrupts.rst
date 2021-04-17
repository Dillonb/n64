Interrupts
==========

Interrupts on the N64 are a multi-layered system. The N64 hardware raises and lowers interrupts through the MIPS interface (MI from now on.) These interrupts can be masked there as well. If an interrupt makes it through this mask, an interrupt will be sent to the CPU.

There are two important registers in the MI that are used for interrupt handling. MI_INTR_REG, and MI_INTR_MASK_REG.

MI_INTR_REG is not writable by the program. Each bit is instead controlled individually by a different component's interrupt. For example, the peripheral interface (PI) will set bit 4 in MI_INTR_REG when a PI DMA completes, and hold it high until it is lowered through a write to PI_STATUS_REG.

MI_INTR_MASK_REG *is* writable by the program, and is used to enable and disable certain interrupts. If a programmer wishes to ignore PI interrupts, they will set bit 4 of MI_INTR_MASK_REG to 0, and to 1 if they wish to enable PI interrupts.

In hardware, this is implemented as a circuit that outputs a 1 when any two corresponding bits are both 1. Most likely with a series of AND gates tied together with a single OR gate, if I had to guess. Because of this, the interrupts are *technically* checked every cycle, but in an emulator you only need to check when either register is written. The check can be implemented like this. Remember, none of this is happening inside the CPU.

.. code-block:: c

   // Should we send an interrupt to the CPU?
   bool interrupt_fired = (MI_INTR_REG & MI_INTR_MASK_REG) != 0;

Now, we're ready to talk about what happens inside the CPU.

The MIPS CPU inside the N64 has eight possible interrupts that can be requested. These correspond to the eight bits in the "Interrupt Pending" field in the COP0 $Cause register.

+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| Bit                  | Description                                                                                                                                        |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 0 (bit 8 of $Cause)  | ip0 - This is writable by MTC0, and is used as a "software interrupt"                                                                              |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 1 (bit 9 of $Cause)  | ip1 - This is writable by MTC0, and is used as a "software interrupt"                                                                              |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 2 (bit 10 of $Cause) | ip2 - This is connected to the MI interrupt process described above. It is set to 1 when (MI_INTR_REG & MI_INTR_MASK_REG) != 0                     |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 3 (bit 11 of $Cause) | ip3 - This is connected to the cartridge slot. Cartridges with special hardware can trigger this interrupt. Unsure how common this is in practice. |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 4 (bit 12 of $Cause) | ip4 - This is connected to the Reset button on the top of the console. When pressed, this becomes 1.                                               |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 5 (bit 13 of $Cause) | ip5 - Connected to the Indy dev kit's RDB port. Set to 1 when a value is read.                                                                     |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 6 (bit 14 of $Cause) | ip6 - Connected to the Indy dev kit's RDB port. Set to 1 when a value is written.                                                                  |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+
| 7 (bit 15 of $Cause) | ip7 - This is connected to the $Count/$Compare interrupt mechanism inside COP0, described in the COP0 Timing Registers section.                    |
+----------------------+----------------------------------------------------------------------------------------------------------------------------------------------------+

As with MI_INTR_REG and MI_INTR_MASK_REG, the interrupt pending field also has a corresponding mask field. It's located at bits 8-15 of $Status. Again, as with the MI registers, these two fields are &'d together, and an interrupt is serviced if the two have any corresponding bits both set to 1.

To stop an interrupt being serviced over and over again in an endless loop, there are additional bits checked in addition to IP & IM. The bits checked are the IE, EXL, and ERL bits in $Status.

We only want to handle interrupts if they are enabled, we're not currently handling an exception, and we're not currently handling an error.  Thus, the full condition for an interrupt being serviced is:

.. code-block:: c

   bool interrupts_pending = (status.im & cause.ip) != 0;
   bool interrupts_enabled = status.ie == 1;
   bool currently_handling_exception = status.exl == 1;
   bool currently_handling error = status.erl == 1;

   bool should_service_interrupt = interrupts_pending
                                       && interrupts_enabled
                                       && !currently_handling_exception
                                       && !currently_handling_error;

This condition is checked every cycle, but can be optimized to be only re-checked when any of these bits change.

When an interrupt is determined to be serviced, an exception is thrown, with exception code 0, meaning interrupt, and no coprocessor error. Exceptions are described in the next section.

Exceptions
==========

Exceptions are how the N64 handles both errors in instructions and interrupts.

Exception Codes
---------------

There are a lot of exceptions. It's worth noting that to get games booting, you pretty much only need the interrupt exception. To get games fully working, you need the interrupt exception and the coprocessor unusable exception for COP1. Games will boot without the COP1 unusuable exception, but will have mild to serious glitches.

+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Name                          | Code | Cop. Err  | Description                                                                                                                  |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Interrupt                     | 0    | Undefined | Thrown when an interrupt occurs.                                                                                             |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| TLB Modification              | 1    | Undefined | Thrown when a TLB page marked read-only is written to                                                                        |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| TLB Miss - Load               | 2    | Undefined | Thrown when no valid TLB entry is found when translating an address to be used for a load (instruction fetch or data access) |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| TLB Miss - Store              | 3    | Undefined | Thrown when no valid TLB entry is found when translating an address to be used for a store (data access)                     |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Address Error - Load          | 4    | Undefined | Thrown when data or an instruction is loaded from an unaligned address.                                                      |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Address Error - Store         | 5    | Undefined | Thrown when data is stored to an unaligned address.                                                                          |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Bus Error - Instruction Fetch | 6    | Undefined | Hardware bus error (timeouts, data corruption, invalid physical memory addresses) when fetching an instruction.              |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Bus Error - Load/Store        | 7    | Undefined | Hardware bus error (timeouts, data corruption, invalid physical memory addresses) when loading or storing data.              |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Syscall                       | 8    | Undefined | Thrown by the SYSCALL MIPS instruction.                                                                                      |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Breakpoint                    | 9    | Undefined | Thrown by the BREAK MIPS instruction                                                                                         |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Reserved Instruction          | 10   | Undefined | Thrown when an invalid instruction is executed. Details below.                                                               |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Coprocessor Unusable          | 11   | Cop. used | Thrown when a coprocessor instruction is used when that coprocessor is disabled. Note that COP0 is never disabled.           |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Arithmetic Overflow           | 12   | Undefined | Thrown by arithmetic instructions when their operations overflow.                                                            |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Trap                          | 13   | Undefined | Thrown by the TRAP family of MIPS instructions.                                                                              |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Floating Point                | 15   | Undefined | Thrown by the FPU when an error case is hit.                                                                                 |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+
| Watch                         | 23   | Undefined | Thrown when a load or store matches the address specified in the COP0 $WatchLo and $WatchHi registers.                       |
+-------------------------------+------+-----------+------------------------------------------------------------------------------------------------------------------------------+

Reserved Instruction Exception cases:
  - Undefined opcode
  - Undefined SPECIAL sub-opcode
  - Undefined REGIMM sub-opcode
  - 64 bit operation run in 32 bit mode. Note that in kernel mode, 64 bit operations are *always valid*, regardless if KX (enable 64 bit addressing in kernel mode) is set or not.

Exception Handling Process
--------------------------

When an exception is thrown, the CPU will update some state inside COP0, and set the program counter to the address of the appropriate exception handler. This address varies depending on the type of exception, and on some state within COP0.

Here is a description on what happens, step by step.

1. If the program counter is currently inside a branch delay slot, set the branch delay bit in $Cause (bit 31) to 1. Otherwise, set this bit to 0.
2. If the EXL bit is currently 0, set the $EPC register in COP0 to the current PC. Then, set the EXL bit to 1.
   A. If we are currently in a branch delay slot, instead set EPC to the address of the *branch that we are currently in the delay slot of, i.e. current_pc - 4.*
3. Set the exception code bit in the COP0 $Cause register to the code of the exception that was thrown.
4. If the coprocessor error is a defined value, i.e. for the coprocessor unusable exception, set the coprocessor error field in $Cause to the coprocessor that caused the error. Otherwise, the value of this field is undefined behavior in hardware, so it *shouldn't* matter what you emulate this as.
5. Jump to the exception vector. A detailed description on how to find the correct exception vector is found on pages 180 through 181 of the manual, and described in less detail below.
   A. Note that there is no "delay slot" executed when jumping to the exception vector, execution jumps there immediately.

Exception Vector Locations
--------------------------

Note that all of these addresses are sign extended to 64 bits.

The reset and NMI exceptions always jump to 0xBFC0'0000. You'll note that this is the base address of the PIF ROM - jumping here will start execution over from scratch.

The locations of the rest of the vectors depend on the BEV bit. This bit is set by the boot process to let hardware know how much of the system's initialization has happened. If BEV=1, we are early in the boot process, and exception vectors should use different code than they will later on. I personally have never run into an exception early enough in the boot process for BEV to be 1, but it's good to check it anyway, just in case.

When BEV is 0, exceptions are handled in a cached region, since it's assumed the cache has already been initialized.
    - 32 bit TLB exceptions jump to 0x8000'0000 when EXL = 0, and 0x8000'0180 when EXL = 1.
    - 64 bit TLB exceptions jump to 0x8000'0080 when EXL = 1, and 0x8000'0180 when EXL = 1.
    - All other exceptions jump to 0x8000'0180.

When BEV is 1, exceptions are handled in an *uncached* region, since it's assumed the cache has *not* been initialized yet.
    - 32 bit TLB exceptions jump to 0xBFC0'0200 when EXL = 0, and 0xBFC0'0380 when EXL = 1.
    - 64 bit TLB exceptions jump to 0xBFC0'0280 when EXL = 1, and 0xBFC0'0380 when EXL = 1.
    - All other exceptions jump to 0xBFC0'0380.
