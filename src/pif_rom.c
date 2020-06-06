#include "pif_rom.h"
#include "common/log.h"
#include "n64bus.h"

void pif_rom_execute(n64_system_t* system) {
    // TODO set registers
    // TODO set CP0 registers

    n64_write_word(0x04300004, 0x01010101);

    // Copy the first 0x1000 bytes of the cartridge to 0xA4000000

    word src_ptr  = 0xB0000000;
    word dest_ptr = 0xA4000000;

    for (int i = 0; i < 0x1000; i++) {
        n64_write_byte(dest_ptr + i, n64_read_byte(src_ptr + i));
    }

    // TODO set pc to 0xA4000040
}
