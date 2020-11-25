#include "dma.h"
#include "n64bus.h"

bool is_dma_active() {
    return false; // DMAs are instant, so we hardcode this for now.
}

void run_dma(n64_system_t* system, word source, word dest, word length, const char* direction) {
    logdebug("DMA requested at PC 0x%08X from 0x%08X to 0x%08X (%s), with a length of %d",
             system->cpu.pc, source, dest, direction, length);
    if (length % 8 == 0) {
        for (int i = 0; i < (length >> 3); i++) {
            dword value = n64_read_dword(system, source + (i << 3));
            logtrace("%s: Copying 0x%016lX from 0x%08X to 0x%08X", direction, value, source + (i << 3), dest + (i << 3));
            n64_write_dword(system, dest + (i << 3), value);
        }
    } else {
        for (int i = 0; i < length; i++) {
            byte value = n64_read_byte(system, source + i);
            logtrace("%s: Copying 0x%02X from 0x%08X to 0x%08X", direction, value, source + i, dest + i);
            n64_write_byte(system, dest + i, value);
        }
    }
    logdebug("DMA completed.");
}
