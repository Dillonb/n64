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

void write_word_vireg(word address, word value) {
    switch (address) {
        case ADDR_VI_STATUS_REG: {
            n64sys.vi.status.raw = value;
            break;
        }
        case ADDR_VI_ORIGIN_REG:
            n64sys.vi.vi_origin = value & 0xFFFFFF;
            loginfo("VI origin is now 0x%08X (wrote 0x%08X)", value & 0xFFFFFF, value);
            break;
        case ADDR_VI_WIDTH_REG: {
            n64sys.vi.vi_width = value & 0x7FF;
            loginfo("VI width is now 0x%X (wrote 0x%08X)", value & 0xFFF, value);
            break;
        }
        case ADDR_VI_V_INTR_REG:
            n64sys.vi.vi_v_intr = value & 0x3FF;
            loginfo("VI interrupt is now 0x%X (wrote 0x%08X) will VI interrupt when v_current == %d", value & 0x3FF, value, value >> 1);
            break;
        case ADDR_VI_V_CURRENT_REG:
            loginfo("V_CURRENT written, V Intr cleared");
            interrupt_lower(INTERRUPT_VI);
            break;
        case ADDR_VI_BURST_REG:
            n64sys.vi.vi_burst.raw = value;
            break;
        case ADDR_VI_V_SYNC_REG:
            n64sys.vi.vsync = value & 0x3FF;
            if (n64sys.vi.vsync != 0x20D) {
                if (n64sys.vi.vsync == 0x271) {
                    logfatal("Wrote 0x%X to VI_VSYNC: currently, only standard NTSC is supported (0x20D.) This looks like a PAL ROM. These are currently not supported.", n64sys.vi.vsync);
                } else if (n64sys.vi.vsync == 0x20C) {
                    logwarn("Wrote 0x20C to VI_VSYNC, this is (valid NTSC value) - 1, I've seen some games do this, no idea why, ignoring...");
                } else {
                    logfatal("Wrote 0x%X to VI_VSYNC: currently, only standard NTSC is supported (0x20D)", n64sys.vi.vsync);
                }
            }
            loginfo("VI vsync is now 0x%X / %d. VSYNC happens on halfline %d (wrote 0x%08X)", value & 0x3FF, value & 0x3FF, (value & 0x3FF) >> 1, value);
            break;
        case ADDR_VI_H_SYNC_REG:
            n64sys.vi.hsync = value & 0x3FF;
            loginfo("VI hsync is now 0x%X (wrote 0x%08X)", value & 0x3FF, value);
            break;
        case ADDR_VI_LEAP_REG:
            n64sys.vi.leap = value;
            loginfo("VI leap is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_H_START_REG:
            n64sys.vi.hstart = value;
            loginfo("VI hstart is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_V_START_REG:
            n64sys.vi.vstart.raw = value;
            break;
        case ADDR_VI_V_BURST_REG:
            n64sys.vi.vburst = value;
            loginfo("VI vburst is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_X_SCALE_REG:
            n64sys.vi.xscale = value;
            loginfo("VI xscale is now 0x%X (wrote 0x%08X)", value, value);
            break;
        case ADDR_VI_Y_SCALE_REG:
            n64sys.vi.yscale = value;
            loginfo("VI yscale is now 0x%X (wrote 0x%08X)", value, value);
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in region: REGION_VI_REGS", value, address);
    }
}

word read_word_vireg(word address) {
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
            return n64sys.vi.v_current << 1;
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

void check_vi_interrupt() {
    if (n64sys.vi.v_current == n64sys.vi.vi_v_intr >> 1) {
        logdebug("Checking for VI interrupt: %d == %d? YES", n64sys.vi.v_current, n64sys.vi.vi_v_intr >> 1);
        interrupt_raise(INTERRUPT_VI);
    } else {
        logdebug("Checking for VI interrupt: %d == %d? nah", n64sys.vi.v_current, n64sys.vi.vi_v_intr >> 1);
    }
}
