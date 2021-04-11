Interrupts
==========

Interrupts on the N64 are a multi-layered system. The N64 hardware raises and lowers interrupts through the MIPS interface (MI from now on.) These interrupts can be masked there as well. If an interrupt makes it through this mask, an interrupt will be sent to the CPU.

There are two important registers in the MI that are used for interrupt handling. MI_INTR_REG, and MI_INTR_MASK_REG.

MI_INTR_REG is not programmer-writeable, and each bit is controlled individually by a different component's interrupt. For example, the peripheral interface (PI) will set bit 4 in MI_INTR_REG when a PI DMA completes, and hold it high until it is lowered through a write to PI_STATUS_REG.

MI_INTR_MASK_REG is programmer-writeable, and is used to enable and disable certain interrupts. If a programmer wishes to ignore PI interrupts, they will set bit 4 of MI_INTR_MASK_REG to 0, and to 1 if they wish to enable PI interrupts.

In hardware, this is implemented as a circuit that outputs a 1 when any two corresponding bits are both 1. Most likely with a series of AND gates tied together with a single OR gate, if I had to guess. Because of this, the interrupts are *technically* checked every cycle, but in an emulator you only need to check when either register is written. The check can be implemented like this. Remember, none of this is happening inside the CPU.

.. code-block:: c

   // Should we send an interrupt to the CPU?
   bool interrupt_fired = (MI_INTR_REG & MI_INTR_MASK_REG) != 0;

Now, we're ready to talk about what happens inside the CPU.

The MIPS CPU inside the N64 has eight possible interrupts that can be requested. These correspond to the eight bits in the "Interrupt Pending" field in the CP0 $Cause register.

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
| 7 (bit 15 of $Cause) | ip7 - This is connected to the $Count/$Compare interrupt mechanism inside CP0, described in the CP0 Timing Registers section.                      |
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

TODO
