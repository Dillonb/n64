#include "rdp.h"
#include <mem/mem_util.h>

#ifndef N64_WIN
#include <dlfcn.h>
#endif
#ifdef N64_MACOS
#include <limits.h>
#else

#endif
#include <stdbool.h>

#include "parallel_rdp_wrapper.h"
#include "softrdp.h"
#include <log.h>
#include <frontend/render.h>
#include <rsp.h>
#include <frontend/frontend.h>

static void* plugin_handle = NULL;
static u32 rdram_size_word = N64_RDRAM_SIZE; // GFX_INFO needs this to be sent as a uint32

#define RDP_COMMAND_BUFFER_SIZE 0xFFFFF
u32 rdp_command_buffer[RDP_COMMAND_BUFFER_SIZE];

#define FROM_RDRAM(address) word_from_byte_array(n64sys.mem.rdram, WORD_ADDRESS(address))
#define FROM_DMEM(address) be32toh(word_from_byte_array(N64RSP.sp_dmem, (address) & 0xFFF))


static const int command_lengths[64] = {
        2, 2, 2, 2, 2, 2, 2, 2, 8, 12, 24, 28, 24, 28, 40, 44,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 4, 4, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2
};

#define RDP_COMMAND_FULL_SYNC 0x29


void rdp_rendering_callback(int redrawn) {
    n64_poll_input();
    n64_render_screen();
}

#define ADDR_DPC_START_REG    0x04100000
#define ADDR_DPC_END_REG      0x04100004
#define ADDR_DPC_CURRENT_REG  0x04100008
#define ADDR_DPC_STATUS_REG   0x0410000C
#define ADDR_DPC_CLOCK_REG    0x04100010
#define ADDR_DPC_BUFBUSY_REG  0x04100014
#define ADDR_DPC_PIPEBUSY_REG 0x04100018
#define ADDR_DPC_TMEM_REG     0x0410001C


#define LOAD_SYM(var, name) do { \
    var = dlsym(plugin_handle, name); \
        if (var == NULL) { logfatal("Failed to load RDP plugin! Missing symbol: %s", name); } \
    } while(false)

void rdp_check_interrupts() {
    on_interrupt_change();
}

void write_word_dpcreg(u32 address, u32 value) {
    switch (address) {
        case ADDR_DPC_START_REG:
            rdp_start_reg_write(value);
            break;
        case ADDR_DPC_END_REG:
            rdp_end_reg_write(value);
            break;
        case ADDR_DPC_CURRENT_REG:
            logwarn("Writing word to read-only DPC register: ADDR_DPC_CURRENT_REG, ignoring");
            break;
        case ADDR_DPC_STATUS_REG:
            rdp_status_reg_write(value);
            break;
        case ADDR_DPC_CLOCK_REG:
            logwarn("Writing word to unimplemented DPC register: ADDR_DPC_CLOCK_REG, ignoring");
            break;
        case ADDR_DPC_BUFBUSY_REG:
            logwarn("Writing word to unimplemented DPC register: ADDR_DPC_BUFBUSY_REG, ignoring");
            break;
        case ADDR_DPC_PIPEBUSY_REG:
            logwarn("Writing word to unimplemented DPC register: ADDR_DPC_PIPEBUSY_REG, ignoring");
            break;
        case ADDR_DPC_TMEM_REG:
            logwarn("Writing word to unimplemented DPC register: ADDR_DPC_TMEM_REG, ignoring");
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address);
    }
}

u32 read_word_dpcreg(u32 address) {
    switch (address) {
        case ADDR_DPC_START_REG:
            return n64sys.dpc.start;
        case ADDR_DPC_END_REG:
            return n64sys.dpc.end;
        case ADDR_DPC_CURRENT_REG:
            return n64sys.dpc.current;
        case ADDR_DPC_STATUS_REG:
            return n64sys.dpc.status.raw;
        case ADDR_DPC_CLOCK_REG:
            return n64sys.dpc.clock;
        case ADDR_DPC_BUFBUSY_REG:
            return n64sys.dpc.status.buf_busy;
        case ADDR_DPC_PIPEBUSY_REG:
            return n64sys.dpc.status.pipe_busy;
        case ADDR_DPC_TMEM_REG:
            return n64sys.dpc.tmem;
        default:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
    }
}

INLINE void rdp_enqueue_command(int command_length, u32* buffer) {
    switch (n64sys.video_type) {
        case UNKNOWN_VIDEO_TYPE:
            logfatal("RDP enqueue command with video type UNKNOWN_VIDEO_TYPE");
        case VULKAN_VIDEO_TYPE:
        case QT_VULKAN_VIDEO_TYPE:
            prdp_enqueue_command(command_length, buffer); break;
        case SOFTWARE_VIDEO_TYPE:
            softrdp_enqueue_command(&n64sys.softrdp_state, command_length, (uint64_t *) buffer); break;
    }
}

INLINE void rdp_on_full_sync() {
    switch (n64sys.video_type) {
        case UNKNOWN_VIDEO_TYPE:
            logfatal("RDP on full sync with video type UNKNOWN_VIDEO_TYPE");
        case VULKAN_VIDEO_TYPE:
        case QT_VULKAN_VIDEO_TYPE:
            prdp_on_full_sync(); break;
        case SOFTWARE_VIDEO_TYPE:
            full_sync_softrdp();
            break;
    }
    n64sys.dpc.status.pipe_busy = false;
    n64sys.dpc.status.start_gclk = false;
    n64sys.dpc.status.cbuf_ready = false;
    interrupt_raise(INTERRUPT_DP);
}

void process_rdp_list() {
    static int last_run_unprocessed_words = 0;

    n64_dpc_t* dpc = &n64sys.dpc;

    // tell the game to not touch RDP stuff while we work
    dpc->status.freeze = true;

    // force align to 8 byte boundaries
    const u32 current = dpc->current & 0x00FFFFF8;
    const u32 end = dpc->end & 0x00FFFFF8;

    // How many bytes do we need to process?
    int display_list_length = end - current;

    if (display_list_length <= 0) {
        // No commands to run
        return;
    }

    if (display_list_length + (last_run_unprocessed_words * 4) > RDP_COMMAND_BUFFER_SIZE) {
        logfatal("Got a command of display_list_length %d / 0x%X (with %d unprocessed words from last run) - this overflows our buffer of size %d / 0x%X!",
                 display_list_length, display_list_length, last_run_unprocessed_words, RDP_COMMAND_BUFFER_SIZE, RDP_COMMAND_BUFFER_SIZE);
    }

    // read the commands into a buffer, 32 bits at a time.
    // we need to read the whole thing into a buffer before sending each command to the RDP
    // because commands have variable lengths
    if (dpc->status.xbus_dmem_dma) {
        for (int i = 0; i < display_list_length; i += 4) {
            u32 command_word = FROM_DMEM(current + i);
            rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    } else {
        if (end > 0x7FFFFFF || current > 0x7FFFFFF) {
            logwarn("Not running RDP commands, wanted to read past end of RDRAM!");
            return;
        }
        for (int i = 0; i < display_list_length; i += 4) {
            u32 command_word = FROM_RDRAM(current + i);
            rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    }

    int length_words = (display_list_length >> 2) + last_run_unprocessed_words;
    int buf_index = 0;

    bool processed_all = true;

    while (buf_index < length_words) {
        u8 command = (rdp_command_buffer[buf_index] >> 24) & 0x3F;

        int command_length = command_lengths[command];

        // Check we actually have enough bytes left in the display list for this command, and save the remainder of the display list for the next run, if not.
        if ((buf_index + command_length) * 4 > display_list_length + (last_run_unprocessed_words * 4)) {
            // Copy remaining bytes back to the beginning of the display list, and save them for next run.
            last_run_unprocessed_words = length_words - buf_index;

            // Safe to allocate this on the stack because we'll only have a partial command left, and that _has_ to be pretty small.
            u32 temp[last_run_unprocessed_words];
            for (int i = 0; i < last_run_unprocessed_words; i++) {
                temp[i] = rdp_command_buffer[buf_index + i];
            }

            for (int i = 0; i < last_run_unprocessed_words; i++) {
                rdp_command_buffer[i] = temp[i];
            }

            processed_all = false;

            break;
        }


        // Don't need to process commands under 8
        if (command >= 8) {
            rdp_enqueue_command(command_length, &rdp_command_buffer[buf_index]);
        }

        if (command == RDP_COMMAND_FULL_SYNC) {
            rdp_on_full_sync();
        }

        buf_index += command_length;
    }

    if (processed_all) {
        last_run_unprocessed_words = 0;
    }

    dpc->current = end;
    dpc->end = end;

    dpc->status.freeze = false;
}

void rdp_run_command() {
    if (n64sys.dpc.status.freeze) {
        return;
    }
    n64sys.dpc.status.pipe_busy = true;
    n64sys.dpc.status.start_gclk = true;

    if (n64sys.dpc.end > n64sys.dpc.current) {
        switch (n64sys.video_type) {
            case VULKAN_VIDEO_TYPE:
            case QT_VULKAN_VIDEO_TYPE:
            case SOFTWARE_VIDEO_TYPE:
                process_rdp_list();
                break;
            default:
                logfatal("Unknown video type");
        }
    }

    n64sys.dpc.status.cbuf_ready = true;
}

void rdp_update_screen() {
    switch (n64sys.video_type) {
        case VULKAN_VIDEO_TYPE:
        case QT_VULKAN_VIDEO_TYPE:
            prdp_update_screen();
            break;
        case SOFTWARE_VIDEO_TYPE:
            n64_render_screen();
            break;
        default:
            logfatal("Unknown video type");
    }
}

void rdp_status_reg_write(u32 value) {
    bool rdp_unfrozen = false;
    union {
        u32 raw;
        struct {
            bool clear_xbus_dmem_dma:1;
            bool set_xbus_dmem_dma:1;
            bool clear_freeze:1;
            bool set_freeze:1;
            bool clear_flush:1;
            bool set_flush:1;
            bool clear_tmem_ctr:1;
            bool clear_pipe_ctr:1;
            bool clear_cmd_ctr:1;
            bool clear_clock_ctr:1;
            unsigned:22;
        } PACKED;
    } status_write;

    status_write.raw = value;

    if (status_write.clear_xbus_dmem_dma) {
        n64sys.dpc.status.xbus_dmem_dma = false;
    }
    if (status_write.set_xbus_dmem_dma) {
        n64sys.dpc.status.xbus_dmem_dma = true;
    }

    if (status_write.clear_freeze) {
        n64sys.dpc.status.freeze = false;
        rdp_unfrozen = true;
    }
    if (status_write.set_freeze) {
        n64sys.dpc.status.freeze = true;
    }

    if (status_write.clear_flush) {
        n64sys.dpc.status.flush = false;
    }
    if (status_write.set_flush) {
        n64sys.dpc.status.flush = true;
    }

    if (status_write.clear_tmem_ctr) {
        n64sys.dpc.status.tmem_busy = false;
    }
    if (status_write.clear_pipe_ctr) {
        n64sys.dpc.status.pipe_busy = false;
    }
    if (status_write.clear_cmd_ctr) {
        n64sys.dpc.status.buf_busy = false;
    }
    if (status_write.clear_clock_ctr) {
        n64sys.dpc.clock = 0;
    }

    if (rdp_unfrozen) {
        rdp_run_command();
    }
}

void rdp_start_reg_write(u32 value) {
    if (!n64sys.dpc.status.start_valid) {
        n64sys.dpc.start = value & 0xFFFFF8;
    }
    n64sys.dpc.status.start_valid = true;
}

void rdp_end_reg_write(u32 value) {
    n64sys.dpc.end = value & 0xFFFFF8;
    if (n64sys.dpc.status.start_valid) {
        n64sys.dpc.current = n64sys.dpc.start;
        n64sys.dpc.status.start_valid = false;
    }
    rdp_run_command();
}
