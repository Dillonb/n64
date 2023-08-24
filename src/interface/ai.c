#include "ai.h"
#include <mem/addresses.h>
#include <frontend/audio.h>
#include <mem/mem_util.h>

void write_word_aireg(u32 address, u32 value) {
    switch (address) {
        case ADDR_AI_DRAM_ADDR_REG:
            if (n64sys.ai.dma_count < 2) {
                n64sys.ai.dma_address[n64sys.ai.dma_count] = value & 0x00FFFFFF & ~7;
            }
            break;
        case ADDR_AI_LEN_REG: {
            u32 length = value & 0b111111111111111111 & ~7;
            if (n64sys.ai.dma_count < 2 && length) {
                n64sys.ai.dma_length[n64sys.ai.dma_count] = length;
                n64sys.ai.dma_count++;
            }
            break;
        }
        case ADDR_AI_CONTROL_REG:
            n64sys.ai.dma_enable = value & 1;
            break;
        case ADDR_AI_STATUS_REG:
            interrupt_lower(INTERRUPT_AI);
            break;
        case ADDR_AI_DACRATE_REG: {
            u32 old_dac_frequency = n64sys.ai.dac.frequency;
            n64sys.ai.dac_rate = value & 0b11111111111111;
            n64sys.ai.dac.frequency = MAX(1, CPU_HERTZ / 2 / (n64sys.ai.dac_rate + 1)) * 1.037;
            n64sys.ai.dac.period = CPU_HERTZ / n64sys.ai.dac.frequency;
            if (old_dac_frequency != n64sys.ai.dac.frequency) {
                adjust_audio_sample_rate(n64sys.ai.dac.frequency);
            }
            break;
        }
        case ADDR_AI_BITRATE_REG:
            n64sys.ai.bitrate = value & 0b1111;
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address);
    }
}

u32 read_word_aireg(u32 address) {
    switch (address) {
        case ADDR_AI_DRAM_ADDR_REG:
            logfatal("Read from unknown AI register: AI_DRAM_ADDR_REG");
        case ADDR_AI_LEN_REG:
            return n64sys.ai.dma_length[0];
        case ADDR_AI_CONTROL_REG:
            logfatal("Read from unknown AI register: AI_CONTROL_REG");
        case ADDR_AI_STATUS_REG: {
            u32 value = 0;
            value |= (n64sys.ai.dma_count > 1) << 0;
            value |= (1) << 20;
            value |= (1) << 24;
            value |= (n64sys.ai.dma_count > 0) << 30;
            value |= (n64sys.ai.dma_count > 1) << 31;
            return value;
        }
            logfatal("Read from unknown AI register: AI_STATUS_REG");
        case ADDR_AI_DACRATE_REG:
            logfatal("Read from unknown AI register: AI_DACRATE_REG");
        case ADDR_AI_BITRATE_REG:
            logfatal("Read from unknown AI register: AI_BITRATE_REG");
        default:
            logfatal("Unrecognized read from AI register: 0x%08X", address);
    }
}

INLINE void sample() {
    if (n64sys.ai.dma_count == 0) {
        audio_push_sample(0, 0);
        return;
    }

    u32 address_hi = ((n64sys.ai.dma_address[0] >> 13) + n64sys.ai.dma_address_carry) & 0x7ff;
    n64sys.ai.dma_address[0] = (address_hi << 13) | n64sys.ai.dma_address[0] & 0x1fff;
    u32 data = RDRAM_WORD(n64sys.ai.dma_address[0]);

    s16 left  = data >> 16;
    s16 right = data >> 0;
    audio_push_sample(left, right);

    u32 address_lo = (n64sys.ai.dma_address[0] + 4) & 0x1fff;
    n64sys.ai.dma_address[0] = (n64sys.ai.dma_address[0] & ~0x1fff) | address_lo;
    n64sys.ai.dma_address_carry = (address_lo == 0);
    n64sys.ai.dma_length[0] -= 4;

    if(!n64sys.ai.dma_length[0]) {
        interrupt_raise(INTERRUPT_AI);
        if(--n64sys.ai.dma_count > 0) { // If we have another DMA pending, start on that one.
            n64sys.ai.dma_address[0] = n64sys.ai.dma_address[1];
            n64sys.ai.dma_length[0]  = n64sys.ai.dma_length[1];
        }
    }
}

void ai_step(int cycles) {
    n64sys.ai.cycles += cycles;
    while (n64sys.ai.cycles > n64sys.ai.dac.period) {
        sample();
        n64sys.ai.cycles -= n64sys.ai.dac.period;
    }
}