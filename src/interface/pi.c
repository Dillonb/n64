#include <mem/addresses.h>
#include <system/n64system.h>
#include <mem/mem_util.h>
#include <system/scheduler.h>
#include <mem/backup.h>
#include "pi.h"

// 9 cycles measured through $Count
#define PI_DMA_CYCLES_PER_BYTE (9 * 2)

word read_word_pireg(word address) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            return n64sys.mem.pi_reg[PI_DRAM_ADDR_REG];
        case ADDR_PI_CART_ADDR_REG:
            return n64sys.mem.pi_reg[PI_CART_ADDR_REG];
        case ADDR_PI_RD_LEN_REG:
            return n64sys.mem.pi_reg[PI_RD_LEN_REG];
        case ADDR_PI_WR_LEN_REG:
            return n64sys.mem.pi_reg[PI_WR_LEN_REG];
        case ADDR_PI_STATUS_REG: {
            word value = 0;
            value |= (n64sys.pi.dma_busy << 0); // Is PI DMA active?
            value |= (0 << 1); // Is PI IO busy?
            value |= (0 << 2); // PI IO error?
            value |= (n64sys.mi.intr.pi << 3); // PI interrupt?
            return value;
        }
        case ADDR_PI_DOMAIN1_REG:
            return n64sys.mem.pi_reg[PI_DOMAIN1_REG];
        case ADDR_PI_BSD_DOM1_PWD_REG:
            return n64sys.mem.pi_reg[PI_BSD_DOM1_PWD_REG];
        case ADDR_PI_BSD_DOM1_PGS_REG:
            return n64sys.mem.pi_reg[PI_BSD_DOM1_PGS_REG];
        case ADDR_PI_BSD_DOM1_RLS_REG:
            return n64sys.mem.pi_reg[PI_BSD_DOM1_RLS_REG];
        case ADDR_PI_DOMAIN2_REG:
            return n64sys.mem.pi_reg[PI_DOMAIN2_REG];
        case ADDR_PI_BSD_DOM2_PWD_REG:
            return n64sys.mem.pi_reg[PI_BSD_DOM2_PWD_REG];
        case ADDR_PI_BSD_DOM2_PGS_REG:
            return n64sys.mem.pi_reg[PI_BSD_DOM2_PGS_REG];
        case ADDR_PI_BSD_DOM2_RLS_REG:
            return n64sys.mem.pi_reg[PI_BSD_DOM2_RLS_REG];
        default:
            logfatal("Reading word from unknown PI register 0x%08X", address);
    }
}

byte dma_cart_read_byte(word address) {
    switch (address) {
        case REGION_CART_2_1:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logwarn("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_1 - This is the N64DD, returning 0xFF because it is not emulated", address);
            return 0xFF;
        case REGION_CART_2_2:
            return backup_read_byte(address - SREGION_CART_2_2);
        case REGION_CART_1_2: {
            word index = BYTE_ADDRESS(address) - SREGION_CART_1_2;
            if (index > n64sys.mem.rom.size) {
                logwarn("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM! (%ld/0x%lX)", address, index, index, n64sys.mem.rom.size, n64sys.mem.rom.size);
                return 0xFF;
            }
            return n64sys.mem.rom.rom[index];
        }
        default:
            logfatal("PI DMA tried to read from %08X", address);
    }
}

void dma_cart_write_byte(word address, byte value) {
    switch (address) {
        case REGION_CART_2_1:
            if (address == 0x05000020) {
                printf("%c", value);
            } else {
                logwarn("Ignoring byte write in REGION_CART_2_1, this is the N64DD! [%08X]=0x%02X", address, value);
            }
            return;
        case REGION_CART_1_1:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            backup_write_byte(address - SREGION_CART_2_2, value);
            return;
        case REGION_CART_1_2:
            logwarn("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
            break;
        default:
            logfatal("PI DMA tried to write %02X to %08X", value, address);
    }
}

void write_word_pireg(word address, word value) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] = value;
            break;
        case ADDR_PI_CART_ADDR_REG:
            n64sys.mem.pi_reg[PI_CART_ADDR_REG] = value;
            break;
        case ADDR_PI_RD_LEN_REG: {
            word length = (value & 0x00FFFFFF) + 1;
            word cart_addr = n64sys.mem.pi_reg[PI_CART_ADDR_REG] & 0xFFFFFFFE;
            word dram_addr = n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] & 0x007FFFFE;

            if (dram_addr & 0x7) {
                length -= dram_addr & 0x7;
            }
            n64sys.mem.pi_reg[PI_RD_LEN_REG] = length;

            if (cart_addr < SREGION_CART_2_1) {
                logfatal("Cart address too low! 0x%08X masked to 0x%08X\n", n64sys.mem.pi_reg[PI_CART_ADDR_REG], cart_addr);
            }
            if (dram_addr >= SREGION_RDRAM_UNUSED) {
                logfatal("DRAM address too high!");
            }


            logdebug("DMA requested at PC 0x%016lX from 0x%08X to 0x%08X (DRAM to CART), with a length of %d", N64CPU.pc, dram_addr, cart_addr, length);

            // TODO: takes 9 cycles per byte to run in reality
            for (int i = 0; i < length; i++) {
                byte b = RDRAM_BYTE(dram_addr + i);
                logtrace("DRAM to CART: Copying 0x%02X from 0x%08X to 0x%08X", b, dram_addr + i, cart_addr + i);
                dma_cart_write_byte(cart_addr + i, b);
            }

            int complete_in = length * PI_DMA_CYCLES_PER_BYTE;
            scheduler_enqueue_relative(complete_in, SCHEDULER_PI_DMA_COMPLETE);
            n64sys.pi.dma_busy = true;

            logdebug("DMA completed. Scheduled interrupt for %d cycles out.", complete_in);
            n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] = dram_addr + length;
            n64sys.mem.pi_reg[PI_CART_ADDR_REG] = cart_addr + length;
            break;
        }
        case ADDR_PI_WR_LEN_REG: {
            word length = (value & 0x00FFFFFF) + 1;
            word cart_addr = n64sys.mem.pi_reg[PI_CART_ADDR_REG] & 0xFFFFFFFE;
            word dram_addr = n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] & 0x007FFFFE;

            if (dram_addr & 0x7) {
                length -= dram_addr & 0x7;
            }
            n64sys.mem.pi_reg[PI_WR_LEN_REG] = length;

            if (cart_addr < SREGION_CART_2_1) {
                logfatal("Cart address too low! 0x%08X masked to 0x%08X\n", n64sys.mem.pi_reg[PI_CART_ADDR_REG], cart_addr);
            }

            logdebug("DMA requested at PC 0x%016lX from 0x%08X to 0x%08X (CART to DRAM), with a length of %d", N64CPU.pc, cart_addr, dram_addr, length);

            if (is_flash(n64sys.mem.save_type) && cart_addr >= 0x08000000 && cart_addr < 0x08010000) {
                // Special case for Flash DMAs
                cart_addr = 0x08000000 | ((cart_addr & 0xFFFFF) << 1);
            }

            for (int i = 0; i < length; i++) {
                byte b = dma_cart_read_byte(cart_addr + i);
                logtrace("CART to DRAM: Copying 0x%02X from 0x%08X to 0x%08X", b, cart_addr + i, dram_addr + i);
                RDRAM_BYTE(dram_addr + i) = b;
            }

            int complete_in = length * PI_DMA_CYCLES_PER_BYTE;
            scheduler_enqueue_relative(complete_in, SCHEDULER_PI_DMA_COMPLETE);
            n64sys.pi.dma_busy = true;

            logdebug("DMA completed. Scheduled interrupt for %d cycles out.", complete_in);
            n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] = dram_addr + length;
            n64sys.mem.pi_reg[PI_CART_ADDR_REG] = cart_addr + length;
            break;
        }
        case ADDR_PI_STATUS_REG: {
            if (value & 0b10) {
                interrupt_lower(INTERRUPT_PI);
            }
            break;
        }
        case ADDR_PI_DOMAIN1_REG:
            n64sys.mem.pi_reg[PI_DOMAIN1_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM1_PWD_REG:
            n64sys.mem.pi_reg[PI_BSD_DOM1_PWD_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM1_PGS_REG:
            n64sys.mem.pi_reg[PI_BSD_DOM1_PGS_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM1_RLS_REG:
            n64sys.mem.pi_reg[PI_BSD_DOM1_RLS_REG] = value & 0xFF;
            break;
        case ADDR_PI_DOMAIN2_REG:
            n64sys.mem.pi_reg[PI_DOMAIN2_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM2_PWD_REG:
            n64sys.mem.pi_reg[PI_BSD_DOM2_PWD_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM2_PGS_REG:
            n64sys.mem.pi_reg[PI_BSD_DOM2_PGS_REG] = value & 0xFF;
            break;
        case ADDR_PI_BSD_DOM2_RLS_REG:
            n64sys.mem.pi_reg[PI_BSD_DOM2_RLS_REG] = value & 0xFF;
            break;
        default:
            logfatal("Writing word 0x%08X to unknown PI register 0x%08X", value, address);
    }
}

void on_pi_dma_complete() {
    interrupt_raise(INTERRUPT_PI);
    n64sys.pi.dma_busy = false;
}