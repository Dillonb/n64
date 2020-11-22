#include "vi.h"
#include <rdp/rdp.h>

#define ADDR_VI_STATUS_REG    0x04400000
#define ADDR_VI_ORIGIN_REG    0x04400004
#define ADDR_VI_WIDTH_REG     0x04400008
#define ADDR_VI_V_INTR_REG    0x0440000C
#define ADDR_VI_V_CURRENT_REG 0x04400010
#define ADDR_VI_BURST_REG     0x04400014
#define ADDR_VI_V_SYNC_REG    0x04400018
#define ADDR_VI_H_SYNC_REG    0x0440001C
#define ADDR_VI_LEAP_REG      0x04400020
#define ADDR_VI_H_START_REG   0x04400024
#define ADDR_VI_V_START_REG   0x04400028
#define ADDR_VI_V_BURST_REG   0x0440002C
#define ADDR_VI_X_SCALE_REG   0x04400030
#define ADDR_VI_Y_SCALE_REG   0x04400034

void write_word_vireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_VI_STATUS_REG: {
            system->vi.status.raw = value;
            break;
        }
        case ADDR_VI_ORIGIN_REG:
            system->vi.vi_origin = value & 0xFFFFFF;
            loginfo("VI origin is now 0x%08X (wrote 0x%08X)", value & 0xFFFFFF, value);
            break;
        case ADDR_VI_WIDTH_REG: {
            system->vi.vi_width = value & 0x7FF;
            loginfo("VI width is now 0x%X (wrote 0x%08X)", value & 0xFFF, value);
            break;
        }
        case ADDR_VI_V_INTR_REG:
            system->vi.vi_v_intr = value & 0x3FF;
            loginfo("VI interrupt is now 0x%X (wrote 0x%08X) will VI interrupt when v_current == %d", value & 0x3FF, value, value >> 1);
            break;
        case ADDR_VI_V_CURRENT_REG:
            loginfo("V_CURRENT written, V Intr cleared");
            interrupt_lower(system, INTERRUPT_VI);
            break;
        case ADDR_VI_BURST_REG:
            system->vi.vi_burst.raw = value;
            break;
        case ADDR_VI_V_SYNC_REG:
            system->vi.vsync = value & 0x3FF;
            if (system->vi.vsync != 0x20D) {
                if (system->vi.vsync == 0x271) {
                    logfatal("Wrote 0x%X to VI_VSYNC: currently, only standard NTSC is supported (0x20D.) This looks like a PAL ROM. These are currently not supported.", system->vi.vsync);
                } else {
                    logfatal("Wrote 0x%X to VI_VSYNC: currently, only standard NTSC is supported (0x20D)", system->vi.vsync);
                }
            }
            loginfo("VI vsync is now 0x%X / %d. VSYNC happens on halfline %d (wrote 0x%08X)", value & 0x3FF, value & 0x3FF, (value & 0x3FF) >> 1, value);
            break;
        case ADDR_VI_H_SYNC_REG:
            system->vi.hsync = value & 0x3FF;
            loginfo("VI hsync is now 0x%X (wrote 0x%08X)", value & 0x3FF, value);
            break;
        case ADDR_VI_LEAP_REG:
            system->vi.leap = value;
            loginfo("VI leap is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_H_START_REG:
            system->vi.hstart = value;
            loginfo("VI hstart is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_V_START_REG:
            system->vi.vstart.raw = value;
            break;
        case ADDR_VI_V_BURST_REG:
            system->vi.vburst = value;
            loginfo("VI vburst is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_X_SCALE_REG:
            system->vi.xscale = value;
            loginfo("VI xscale is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_Y_SCALE_REG:
            system->vi.yscale = value;
            loginfo("VI yscale is now 0x%X (wrote 0x%08X)", value, value);
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in region: REGION_VI_REGS", value, address);
    }
}

word read_word_vireg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_VI_STATUS_REG:
            logfatal("Reading of ADDR_VI_STATUS_REG is unsupported");
        case ADDR_VI_ORIGIN_REG:
            logfatal("Reading of ADDR_VI_ORIGIN_REG is unsupported");
        case ADDR_VI_WIDTH_REG:
            logfatal("Reading of ADDR_VI_WIDTH_REG is unsupported");
        case ADDR_VI_V_INTR_REG:
            logfatal("Reading of ADDR_VI_V_INTR_REG is unsupported");
        case ADDR_VI_V_CURRENT_REG:
            return system->vi.v_current << 1;
        case ADDR_VI_BURST_REG:
            logfatal("Reading of ADDR_VI_BURST_REG is unsupported");
        case ADDR_VI_V_SYNC_REG:
            logfatal("Reading of ADDR_VI_V_SYNC_REG is unsupported");
        case ADDR_VI_H_SYNC_REG:
            logfatal("Reading of ADDR_VI_H_SYNC_REG is unsupported");
        case ADDR_VI_LEAP_REG:
            logfatal("Reading of ADDR_VI_LEAP_REG is unsupported");
        case ADDR_VI_H_START_REG:
            logfatal("Reading of ADDR_VI_H_START_REG is unsupported");
        case ADDR_VI_V_START_REG:
            logfatal("Reading of ADDR_VI_V_START_REG is unsupported");
        case ADDR_VI_V_BURST_REG:
            logfatal("Reading of ADDR_VI_V_BURST_REG is unsupported");
        case ADDR_VI_X_SCALE_REG:
            logfatal("Reading of ADDR_VI_X_SCALE_REG is unsupported");
        case ADDR_VI_Y_SCALE_REG:
            logfatal("Reading of ADDR_VI_Y_SCALE_REG is unsupported");
        default:
            logfatal("Attempted to read word from unknown VI reg: 0x%08X", address);
    }
}

void check_vi_interrupt(n64_system_t* system) {
    if (system->vi.v_current == system->vi.vi_v_intr >> 1) {
        logdebug("Checking for VI interrupt: %d == %d? YES", system->vi.v_current, system->vi.vi_v_intr >> 1);
        interrupt_raise(INTERRUPT_VI);
    } else {
        logdebug("Checking for VI interrupt: %d == %d? nah", system->vi.v_current, system->vi.vi_v_intr >> 1);
    }
}
