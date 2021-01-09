Issues Log
==========
I'm keeping a small log of various bugs and issues I fixed, categorized under the games they fixed.

Namco Museum
------------
Distorted audio - the game uses COP1 Unusable Exceptions as a trigger to initialize the audio system. If these are not thrown, audio will be extremely distorted.

Super Mario 64
--------------
Distorted audio - the game uses COP1 Unusable Exceptions as a trigger to initialize the audio system. If these are not thrown, audio will be extremely distorted.

Ocarina of Time
---------------
I had major graphical issues in the game when I first got it booting. The game would only draw perfectly horizontal lines across the screen, but otherwise appeared to work fine. This was solved by ensuring that LWC1, LDC1, SWC1, and SDC1 threw COP1 unusable exceptions. I missed this because these instructions have first-level opcodes unlike the other COP1 instructions.

The game would hang upon opening the pause menu. This is because the operating system uses software timers while opening this screen. These were broken because my handling of Compare interrupts was incorrect.

Mario Kart 64
-------------
Game would hang upon completing a Grand Prix. This ended up being because EEPROM wasn't identified correctly in the PIF, when channel 4 was requested in a controller ID command.
