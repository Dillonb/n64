#include "pif_rom.h"
#include "common/log.h"
#include "n64bus.h"

void pif_rom_execute(n64_system_t* system) {
    system->cpu.gpr[0] = 0;
    system->cpu.gpr[1] = 0;
    system->cpu.gpr[2] = 0;
    system->cpu.gpr[3] = 0;
    system->cpu.gpr[4] = 0;
    system->cpu.gpr[5] = 0;
    system->cpu.gpr[6] = 0;
    system->cpu.gpr[7] = 0;
    system->cpu.gpr[8] = 0;
    system->cpu.gpr[9] = 0;
    system->cpu.gpr[10] = 0;
    system->cpu.gpr[11] = 0;
    system->cpu.gpr[12] = 0;
    system->cpu.gpr[13] = 0;
    system->cpu.gpr[14] = 0;
    system->cpu.gpr[15] = 0;
    system->cpu.gpr[16] = 0;
    system->cpu.gpr[17] = 0;
    system->cpu.gpr[18] = 0;
    system->cpu.gpr[19] = 0;
    system->cpu.gpr[20] = 0x1;
    system->cpu.gpr[21] = 0;
    system->cpu.gpr[22] = 0x3F;
    system->cpu.gpr[23] = 0;
    system->cpu.gpr[24] = 0;
    system->cpu.gpr[25] = 0;
    system->cpu.gpr[26] = 0;
    system->cpu.gpr[27] = 0;
    system->cpu.gpr[28] = 0;
    system->cpu.gpr[29] = 0xA4001FF0;
    system->cpu.gpr[30] = 0;
    system->cpu.gpr[31] = 0;

    system->cpu.cp0.r[0] = 0;
    system->cpu.cp0.r[1] = 0x0000001F;
    system->cpu.cp0.r[2] = 0;
    system->cpu.cp0.r[3] = 0;
    system->cpu.cp0.r[4] = 0;
    system->cpu.cp0.r[5] = 0;
    system->cpu.cp0.r[6] = 0;
    system->cpu.cp0.r[7] = 0;
    system->cpu.cp0.r[8] = 0;
    system->cpu.cp0.r[9] = 0;
    system->cpu.cp0.r[10] = 0;
    system->cpu.cp0.r[11] = 0;
    system->cpu.cp0.r[12] = 0x70400004;
    system->cpu.cp0.r[13] = 0;
    system->cpu.cp0.r[14] = 0;
    system->cpu.cp0.r[15] = 0x00000B00;
    system->cpu.cp0.r[16] = 0x0006E463;
    system->cpu.cp0.r[17] = 0;
    system->cpu.cp0.r[18] = 0;
    system->cpu.cp0.r[19] = 0;
    system->cpu.cp0.r[20] = 0;
    system->cpu.cp0.r[21] = 0;
    system->cpu.cp0.r[22] = 0;
    system->cpu.cp0.r[23] = 0;
    system->cpu.cp0.r[24] = 0;
    system->cpu.cp0.r[25] = 0;
    system->cpu.cp0.r[26] = 0;
    system->cpu.cp0.r[27] = 0;
    system->cpu.cp0.r[28] = 0;
    system->cpu.cp0.r[29] = 0;
    system->cpu.cp0.r[30] = 0;
    system->cpu.cp0.r[31] = 0;

    //n64_write_word(system, 0x04300004, 0x01010101);

    // Copy the first 0x1000 bytes of the cartridge to 0xA4000000

    word src_ptr  = 0xB0000000;
    word dest_ptr = 0xA4000000;

    for (int i = 0; i < 0x1000; i++) {
        word src_address = src_ptr + i;
        word dest_address = dest_ptr + i;
        byte src = n64_read_byte(system, src_address);
        n64_write_byte(system, dest_address, src);

        logtrace("PIF: Copied 0x%02X from 0x%08X ==> 0x%08X", src, src_address, dest_address)
    }

    system->cpu.pc = 0xA4000040;
}
