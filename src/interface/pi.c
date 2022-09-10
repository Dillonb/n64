#include <mem/addresses.h>
#include <system/n64system.h>
#include <mem/mem_util.h>
#include <system/scheduler.h>
#include <mem/backup.h>
#include <dynarec/dynarec.h>
#include <timing.h>
#include "pi.h"

u32 read_word_pireg(u32 address) {
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
            u32 value = 0;
            value |= (n64sys.pi.dma_busy << 0); // Is PI DMA active?
            value |= (n64sys.pi.io_busy << 1); // Is PI IO busy?
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

u8 pi_get_domain(u32 address) {
    switch (address) {
        case REGION_CART_1_1:
        case REGION_CART_1_2:
            return 1;
        case REGION_CART_2_1:
        case REGION_CART_2_2:
            return 2;
        default:
            logfatal("Unknown PI domain for address %08X!\n", address);
    }
}

    u8 dma_cart_read_byte(u32 address) {
    switch (address) {
        case REGION_CART_2_1:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logwarn("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_1 - This is the N64DD, returning 0xFF because it is not emulated", address);
            return 0xFF;
        case REGION_CART_2_2:
            return backup_read_byte(address - SREGION_CART_2_2);
        case REGION_CART_1_2: {
            u32 index = BYTE_ADDRESS(address) - SREGION_CART_1_2;
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

void dma_cart_write_byte(u32 address, u8 value) {
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

void write_word_pireg(u32 address, u32 value) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] = value;
            break;
        case ADDR_PI_CART_ADDR_REG:
            n64sys.mem.pi_reg[PI_CART_ADDR_REG] = value;
            break;
        case ADDR_PI_RD_LEN_REG: {
            u32 length = (value & 0x00FFFFFF) + 1;
            u32 cart_addr = n64sys.mem.pi_reg[PI_CART_ADDR_REG] & 0xFFFFFFFE;
            u32 dram_addr = n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] & 0x007FFFFE;

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
                u8 b = RDRAM_BYTE(dram_addr + i);
                logtrace("DRAM to CART: Copying 0x%02X from 0x%08X to 0x%08X", b, dram_addr + i, cart_addr + i);
                dma_cart_write_byte(cart_addr + i, b);
            }

            int complete_in = timing_pi_access(pi_get_domain(cart_addr), length);
            scheduler_enqueue_relative(complete_in, SCHEDULER_PI_DMA_COMPLETE);
            n64sys.pi.dma_busy = true;

            logdebug("DMA completed. Scheduled interrupt for %d cycles out.", complete_in);
            n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] = dram_addr + length;
            n64sys.mem.pi_reg[PI_CART_ADDR_REG] = cart_addr + length;
            break;
        }
        case ADDR_PI_WR_LEN_REG: {
            u32 length = (value & 0x00FFFFFF) + 1;
            u32 cart_addr = n64sys.mem.pi_reg[PI_CART_ADDR_REG] & 0xFFFFFFFE;
            u32 dram_addr = n64sys.mem.pi_reg[PI_DRAM_ADDR_REG] & 0x007FFFFE;

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
                u8 b = dma_cart_read_byte(cart_addr + i);
                logtrace("CART to DRAM: Copying 0x%02X from 0x%08X to 0x%08X", b, cart_addr + i, dram_addr + i);
                RDRAM_BYTE(dram_addr + i) = b;
                invalidate_dynarec_page(BYTE_ADDRESS(dram_addr + i));
            }

            u32 begin_index = dynarec_outer_index(dram_addr);
            u32 end_index = dynarec_outer_index(dram_addr + length);

            for (u32 i = begin_index; i <= end_index; i++) {
                invalidate_dynarec_page_by_index(i);
            }

            int complete_in = timing_pi_access(pi_get_domain(cart_addr), length);
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

INLINE bool pi_write_latch(u32 value) {
    if (n64sys.pi.io_busy) {
        return false;
    } else {
        n64sys.pi.io_busy = true;
        n64sys.pi.latch = value;
        scheduler_enqueue_relative(PI_BUS_WRITE, SCHEDULER_PI_BUS_WRITE_COMPLETE);
        return true;
    }
}

INLINE bool pi_read_latch() {
    if (unlikely(n64sys.pi.io_busy)) {
        n64sys.pi.io_busy = false;
        cpu_stall(scheduler_remove_event(SCHEDULER_PI_BUS_WRITE_COMPLETE));
        return false;
    }
    return true;
}

void write_byte_pibus(u32 address, u32 value) {
    // If the write is misaligned, cache 16 bits instead of 8
    int latch_shift = 24 - (address & 1) * 8;
    if (!pi_write_latch(value << latch_shift)) {
        return; // Couldn't latch, ignore the write.
    }
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
            logfatal("write_byte_pibus(): Access to non-PI address %08X", address);
    }

}

u8 read_byte_pibus(u32 address) {
    if (unlikely(!pi_read_latch())) {
        return n64sys.pi.latch >> 24;
    }

    switch (address) {
        case REGION_CART_2_1:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logwarn("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_1 - This is the N64DD, returning 0xFF because it is not emulated", address);
            return 0xFF;
        case REGION_CART_2_2:
            return backup_read_byte(address - SREGION_CART_2_2);
        case REGION_CART_1_2: {
            // round to nearest 4 byte boundary, keeping old LSB
            address = (address + 2) & ~2;
            u32 index = BYTE_ADDRESS(address) - SREGION_CART_1_2;
            if (index > n64sys.mem.rom.size) {
                logwarn("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM! (%ld/0x%lX)", address, index, index, n64sys.mem.rom.size, n64sys.mem.rom.size);
                return 0xFF;
            }
            return n64sys.mem.rom.rom[index];
        }
        default:
            logfatal("read_byte_pibus(): Access to non-PI address %08X", address);
    }

}

void write_half_pibus(u32 address, u16 value) {
    if (!pi_write_latch(value << 16)) {
        return; // Couldn't latch, ignore the write.
    }

    switch (address) {
        case REGION_CART_2_1:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_CART_2_1", value, address);
        case REGION_CART_1_1:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            logfatal("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_CART_2_2", value, address);
        case REGION_CART_1_2:
            logwarn("Writing u16 0x%04X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
            break;
        default:
            logfatal("write_half_pibus(): Access to non-PI address %08X", address);
    }
}

u16 read_half_pibus(u32 address) {
    if (unlikely(!pi_read_latch())) {
        return n64sys.pi.latch >> 16;
    }

    switch (address) {
        case REGION_CART_2_1:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_CART_1_1", address);
        case REGION_CART_2_2:
            logfatal("Reading u16 from address 0x%08X in unsupported region: REGION_CART_2_2", address);
        case REGION_CART_1_2: {
            address = (address + 2) & ~3; // round to nearest 4 byte boundary
            u32 index = HALF_ADDRESS(address) - SREGION_CART_1_2;
            if (index > n64sys.mem.rom.size - 1) { // -1 because we're reading an entire u16
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return half_from_byte_array(n64sys.mem.rom.rom, index);
        }
        default:
            logfatal("read_half_pibus(): Access to non-PI address %08X", address);
    }
}

void write_word_pibus(u32 address, u32 value) {
    if (!pi_write_latch(value)) {
        return; // Couldn't latch, ignore the write.
    }

    switch (address) {
        case REGION_CART_2_1:
            logwarn("Writing word 0x%08X to address 0x%08X in region: REGION_CART_1_1, this is the 64DD, ignoring!", value, address);
            return;
        case REGION_CART_1_1:
            logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
            return;
        case REGION_CART_2_2:
            backup_write_word(address - SREGION_CART_2_2, value);
            return;
        case REGION_CART_1_2:
            switch (address) {
                case REGION_CART_ISVIEWER_BUFFER:
                    word_to_byte_array(n64sys.mem.isviewer_buffer, address - SREGION_CART_ISVIEWER_BUFFER, be32toh(value));
                    break;
                case CART_ISVIEWER_FLUSH: {
                    if (value < CART_ISVIEWER_SIZE) {
                        char* message = malloc(value + 1);
                        memcpy(message, n64sys.mem.isviewer_buffer, value);
                        message[value] = '\0';
                        printf("%s", message);
                        free(message);
                    } else {
                        logfatal("ISViewer buffer size is emulated at %d bytes, but received a flush command for %d bytes!", CART_ISVIEWER_SIZE, value);
                    }
                    break;
                }
                default:
                    logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
            }
            return;
        default:
            logfatal("write_word_pibus(): Access to non-PI address %08X", address);
    }
}

u32 read_word_pibus(u32 address) {
    if (unlikely(!pi_read_latch())) {
        return n64sys.pi.latch;
    }
    switch (address) {
        case REGION_CART_2_1:
            logwarn("Reading word from address 0x%08X in unsupported region: REGION_CART_2_1 - This is the N64DD, returning FF because it is not emulated", address);
            return 0xFF;
        case REGION_CART_1_1:
            logwarn("Reading word from address 0x%08X in unsupported region: REGION_CART_1_1 - This is the N64DD, returning FF because it is not emulated", address);
            return 0xFF;
        case REGION_CART_2_2:
            return backup_read_word(address - SREGION_CART_2_2);
        case REGION_CART_1_2: {
            u32 index = WORD_ADDRESS(address) - SREGION_CART_1_2;
            if (index > n64sys.mem.rom.size - 3) { // -3 because we're reading an entire word
                switch (address) {
                    case REGION_CART_ISVIEWER_BUFFER:
                        return htobe32(word_from_byte_array(n64sys.mem.isviewer_buffer, address - SREGION_CART_ISVIEWER_BUFFER));
                    case CART_ISVIEWER_FLUSH:
                        logfatal("Read from ISViewer flush!");
                }
                logwarn("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
                return 0;
            } else {
                return word_from_byte_array(n64sys.mem.rom.rom, index);
            }
        }
        default:
            logfatal("read_word_pibus(): Access to non-PI address %08X", address);
    }
}

void write_dword_pibus(u32 address, u64 value) {
    if (!pi_write_latch(value >> 32)) {
        return; // Couldn't latch, ignore the write.
    }
    switch (address) {
        case REGION_CART_2_1:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_2_1", value, address);
        case REGION_CART_1_1:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_1_1", value, address);
        case REGION_CART_2_2:
            logfatal("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_2_2", value, address);
        case REGION_CART_1_2:
            logwarn("Writing dword 0x%016lX to address 0x%08X in unsupported region: REGION_CART_1_2", value, address);
            break;
        default:
            logfatal("write_dword_pibus(): Access to non-PI address %08X", address);
    }
}

u64 read_dword_pibus(u32 address) {
    if (unlikely(!pi_read_latch())) {
        return (u64)n64sys.pi.latch << 32;
    }
    switch (address) {
        case REGION_CART_2_1:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_2_1", address);
        case REGION_CART_1_1:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_1_1", address);
        case REGION_CART_2_2:
            logfatal("Reading dword from address 0x%08X in unsupported region: REGION_CART_2_2", address);
        case REGION_CART_1_2: {
            u32 index = DWORD_ADDRESS(address) - SREGION_CART_1_2;
            if (index > n64sys.mem.rom.size - 7) { // -7 because we're reading an entire dword
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index);
            }
            return dword_from_byte_array(n64sys.mem.rom.rom, index);
        }
        default:
            logfatal("read_dword_pibus(): Access to non-PI address %08X", address);
    }
}

void on_pi_dma_complete() {
    interrupt_raise(INTERRUPT_PI);
    n64sys.pi.dma_busy = false;
}

void on_pi_write_complete() {
    n64sys.pi.io_busy = false;
}
