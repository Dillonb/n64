#include "ai.h"
#include "../mem/addresses.h"
#include "../frontend/render.h"

INLINE int max(int x, int y) {
    if (x > y) return x;
    return y;
}

void write_word_aireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_AI_DRAM_ADDR_REG:
            if (system->ai.dma_count < 2) {
                system->ai.dma_address[system->ai.dma_count] = value & 0x00FFFFFF & ~7;
            }
            break;
        case ADDR_AI_LEN_REG: {
            word length = value & 0b111111111111111111 & ~7;
            if (system->ai.dma_count < 2 && length) {
                system->ai.dma_length[system->ai.dma_count] = length;
                system->ai.dma_count++;
            }
            break;
        }
        case ADDR_AI_CONTROL_REG:
            system->ai.dma_enable = value & 1;
            break;
        case ADDR_AI_STATUS_REG:
            interrupt_lower(system, INTERRUPT_AI);
            break;
        case ADDR_AI_DACRATE_REG: {
            word old_dac_frequency = system->ai.dac.frequency;
            system->ai.dac_rate = value & 0b11111111111111;
            system->ai.dac.frequency = max(1, CPU_HERTZ / 2 / (system->ai.dac_rate + 1)) * 1.037;
            system->ai.dac.period = CPU_HERTZ / system->ai.dac.frequency;
            if (old_dac_frequency != system->ai.dac.frequency) {
                adjust_audio_sample_rate(system->ai.dac.frequency);
            }
            break;
        }
        case ADDR_AI_BITRATE_REG:
            system->ai.bitrate = value & 0b1111;
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address)
    }
}

word read_word_aireg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_AI_DRAM_ADDR_REG:
            logfatal("Read from unknown AI register: AI_DRAM_ADDR_REG")
        case ADDR_AI_LEN_REG:
            logfatal("Read from unknown AI register: AI_LEN_REG")
        case ADDR_AI_CONTROL_REG:
            logfatal("Read from unknown AI register: AI_CONTROL_REG")
        case ADDR_AI_STATUS_REG: {
            word value = 0;
            value |= (system->ai.dma_count > 1) << 0;
            value |= (1) << 20;
            value |= (1) << 24;
            value |= (system->ai.dma_count > 0) << 30;
            value |= (system->ai.dma_count > 1) << 31;
            return value;
        }
            logfatal("Read from unknown AI register: AI_STATUS_REG")
        case ADDR_AI_DACRATE_REG:
            logfatal("Read from unknown AI register: AI_DACRATE_REG")
        case ADDR_AI_BITRATE_REG:
            logfatal("Read from unknown AI register: AI_BITRATE_REG")
        default:
            logfatal("Unrecognized read from AI register: 0x%08X", address)
    }
}

void sample(n64_system_t* system) {
    if (system->ai.dma_count == 0) {
        return;
    }

    word data  = system->cpu.read_word(system->ai.dma_address[0]);
    shalf left  = data >> 16;
    shalf right = data >>  0;
    audio_push_sample(left, right);

    system->ai.dma_address[0] += 4;
    system->ai.dma_length[0]  -= 4;
    if(!system->ai.dma_length[0]) {
        interrupt_raise(system, INTERRUPT_AI);
        if(--system->ai.dma_count > 0) { // If we have another DMA pending, start on that one.
            system->ai.dma_address[0] = system->ai.dma_address[1];
            system->ai.dma_length[0]  = system->ai.dma_length[1];
        }
    }
}

void ai_step(n64_system_t* system, int cycles) {
    system->ai.cycles += cycles;
    while (system->ai.cycles > system->ai.dac.period) {
        sample(system);
        system->ai.cycles -= system->ai.dac.period;
    }
}