#include <timing.h>
#include <math.h>
#include <system/n64system.h>
#include "log.h"

unsigned int extra_cycles = 0;

// Thanks m64p
u32 timing_pi_access(u8 domain, u32 length)
{
#ifdef INSTANT_PI_DMA
    return 0;
#else
    uint32_t cycles = 0;
    uint32_t latency = 0;
    uint32_t pulse_width = 0;
    uint32_t release = 0;
    uint32_t page_size = 0;
    uint32_t pages = 0;

    switch (domain) {
        case 1:
            latency = n64sys.mem.pi_reg[PI_DOMAIN1_REG] + 1;
            pulse_width = n64sys.mem.pi_reg[PI_BSD_DOM1_PWD_REG] + 1;
            release = n64sys.mem.pi_reg[PI_BSD_DOM1_RLS_REG] + 1;
            page_size = pow(2, (n64sys.mem.pi_reg[PI_BSD_DOM1_PGS_REG] + 2));
            break;
        case 2:
            latency = n64sys.mem.pi_reg[PI_DOMAIN2_REG] + 1;
            pulse_width = n64sys.mem.pi_reg[PI_BSD_DOM2_PWD_REG] + 1;
            release = n64sys.mem.pi_reg[PI_BSD_DOM2_RLS_REG] + 1;
            page_size = pow(2, (n64sys.mem.pi_reg[PI_BSD_DOM2_PGS_REG] + 2));
            break;
        default:
            logfatal("Unknown PI domain: %d\n", domain);
    }

    pages = ceil((double)length / page_size);

    cycles += (14 + latency) * pages;
    cycles += (pulse_width + release) * (length / 2);
    cycles += 5 * pages;
    return cycles * 1.5; // Converting RCP clock speed to CPU clock speed
#endif
}
