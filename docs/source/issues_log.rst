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

Paper Mario
-----------
The game would hang when Mario falls off the veranda behind the goombas' house. The cause ended up being that my DIVU instruction was broken.

DIV is best implemented with 64 bit signed integers, to guard against an INT_MIN / -1 case. When implementing DIVU, I copypasted my DIV implementation and made the 64 bit integers unsigned.

Paper Mario's rand_int() function performs a DIVU with 0xFFFFFFFF / x. My DIVU implementation was calculating this as 0xFFFFFFFF'FFFFFFFF / x. With a signed divide, this is fine, since both of those numbers represent negative one. With an unsigned divide, however, they give different results.

This was causing random event probabilities to be very incorrect, which, long story short, ended up causing a hang.