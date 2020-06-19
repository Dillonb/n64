#include "dma.h"
#include "n64bus.h"

bool is_dma_active() {
    return false; // DMAs are instant, so we hardcode this for now.
}

void run_dma(n64_system_t* system, word source, word dest, word length, const char* direction) {
    logdebug("DMA requested from 0x%08X to 0x%08X (%s), with a length of %d",
             source, dest, direction, length)
    for (int i = 0; i <= length; i++) { // The <= is intentional. The written length value is the length - 1.
        byte value = n64_read_byte(system, source + i);
        logtrace("Copying 0x%02X from 0x%08X to 0x%08X", value, source + i, dest + i);
        n64_write_byte(system, dest + i, value);
    }
    logdebug("DMA completed.")
}
