Issues Log
==========
I'm keeping a small log of various bugs and issues I fixed, categorized under the games they fixed.

Namco Museum & Super Mario 64
-----------------------------
Distorted audio - the operating system uses COP1 Unusable Exceptions as a trigger to save the FPU registers on a context switch.

If these are not thrown, the FPU registers will not be properly saved/restored, and the audio will be extremely distorted due to incorrect values remaining in the registers when the OS switches back to the audio thread.

Ocarina of Time
---------------
I had major graphical issues in the game when I first got it booting. The game would only draw perfectly horizontal lines across the screen, but otherwise appeared to work fine. This was solved by ensuring that LWC1, LDC1, SWC1, and SDC1 threw COP1 unusable exceptions. I missed this because these instructions have first-level opcodes unlike the other COP1 instructions.

The game would hang upon opening the pause menu. This is because the operating system uses software timers while opening this screen. These were broken because my handling of Compare interrupts was incorrect.

Mario Kart 64, other games using EEPROM
---------------------------------------
Game would hang upon completing a Grand Prix. This ended up being because EEPROM wasn't identified correctly in the PIF, when channel 4 was requested in a controller ID command.

F-Zero X
--------
Expects the N64DD's status register at 0x05000508 to return 0xFFFFFFFF if the DD is absent. Or, I assume, the N64DD to be correctly emulated. Otherwise, it will hang indefinitely on a black screen.

The read from the status register is performed at PC value 0x800C5A84. The hang happens at PC value 0x80414CF4.