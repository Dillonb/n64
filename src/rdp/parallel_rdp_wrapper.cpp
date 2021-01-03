#include <rdp_device.hpp>
#include <wsi.hpp>

#include "parallel_rdp_wrapper.h"
#include "rdp.h"
#include <memory>
#include <SDL_video.h>
#include <frontend/render.h>
#include <SDL_vulkan.h>
#include <mem/mem_util.h>

using namespace Vulkan;
using std::unique_ptr;
using RDP::CommandProcessor;
using RDP::CommandProcessorFlags;
using RDP::VIRegister;

static CommandProcessor* command_processor;
static WSI* wsi;

std::vector<Semaphore> acquire_semaphore;

#define RDP_COMMAND_BUFFER_SIZE 0xFFFFF
word parallel_rdp_command_buffer[RDP_COMMAND_BUFFER_SIZE];

extern "C" {
    extern SDL_Window* window;
}

class SDLWSIPlatform : public Vulkan::WSIPlatform {
public:
    SDLWSIPlatform() = default;

    std::vector<const char *> get_instance_extensions() override {
        const char* extensions[64];
        unsigned int num_extensions = 64;

        if (!SDL_Vulkan_GetInstanceExtensions(window, &num_extensions, extensions)) {
            logfatal("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
        }
        auto vec = std::vector<const char*>();

        for (unsigned int i = 0; i < num_extensions; i++) {
            vec.push_back(extensions[i]);
        }

        return vec;
    }

    VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice gpu) override {
        VkSurfaceKHR vk_surface;
        if (!SDL_Vulkan_CreateSurface(window, instance, &vk_surface)) {
            logfatal("Failed to create Vulkan window surface: %s", SDL_GetError());
        }
        return vk_surface;
    }

    uint32_t get_surface_width() override {
        return N64_SCREEN_X * SCREEN_SCALE;
    }

    uint32_t get_surface_height() override {
        return N64_SCREEN_Y * SCREEN_SCALE;
    }

    bool alive(Vulkan::WSI &wsi) override {
        return true;
    }

    void poll_input() override {
        n64_poll_input(global_system);
    }

    void event_frame_tick(double frame, double elapsed) override {
        n64_render_screen(global_system);
    }
};

void load_parallel_rdp(n64_system_t* system) {
    wsi = new WSI();
    wsi->set_backbuffer_srgb(false);
    wsi->set_platform(new SDLWSIPlatform());
    if (!wsi->init(1, nullptr)) {
        logfatal("Failed to initialize WSI!");
    }

    auto aligned_rdram = reinterpret_cast<uintptr_t>(system->mem.rdram);
    uintptr_t offset = 0;

    if (wsi->get_device().get_device_features().supports_external_memory_host)
    {
        size_t align = wsi->get_device().get_device_features().host_memory_properties.minImportedHostPointerAlignment;
        offset = aligned_rdram & (align - 1);
        aligned_rdram -= offset;
    } else {
        logwarn("VK_EXT_external_memory_host is not supported by this device. Application might run slower because of this.");
    }

    CommandProcessorFlags flags = 1 << 3; // TODO configurable scaling

    command_processor = new CommandProcessor(wsi->get_device(), reinterpret_cast<void *>(aligned_rdram),
                                        offset, 8 * 1024 * 1024, 4 * 1024 * 1024, flags);

    if (!command_processor->device_is_supported()) {
        logfatal("This device probably does not support 8/16-bit storage. Make sure you're using up-to-date drivers!");
    }
}

void update_screen_parallel_rdp(n64_system_t* system) {
    if (unlikely(!command_processor)) {
        logfatal("Update screen without an initialized command processor");
    }

    command_processor->set_vi_register(VIRegister::Control,      system->vi.status.raw);
    command_processor->set_vi_register(VIRegister::Origin,       system->vi.vi_origin);
    command_processor->set_vi_register(VIRegister::Width,        system->vi.vi_width);
    command_processor->set_vi_register(VIRegister::Intr,         system->vi.vi_v_intr);
    command_processor->set_vi_register(VIRegister::VCurrentLine, system->vi.v_current);
    command_processor->set_vi_register(VIRegister::Timing,       system->vi.vi_burst.raw);
    command_processor->set_vi_register(VIRegister::VSync,        system->vi.vsync);
    command_processor->set_vi_register(VIRegister::HSync,        system->vi.hsync);
    command_processor->set_vi_register(VIRegister::Leap,         system->vi.leap);
    command_processor->set_vi_register(VIRegister::HStart,       system->vi.hstart);
    command_processor->set_vi_register(VIRegister::VStart,       system->vi.vstart.raw);
    command_processor->set_vi_register(VIRegister::VBurst,       system->vi.vburst);
    command_processor->set_vi_register(VIRegister::XScale,       system->vi.xscale);
    command_processor->set_vi_register(VIRegister::YScale,       system->vi.yscale);

    RDP::ScanoutOptions opts;
    opts.persist_frame_on_invalid_input = true;
    opts.vi.aa = true;
    opts.vi.scale = true;
    opts.vi.dither_filter = true;
    opts.vi.divot_filter = true;
    opts.vi.gamma_dither = true;
    opts.downscale_steps = true;
    opts.crop_overscan_pixels = true;
    Util::IntrusivePtr<Image> image = command_processor->scanout(opts);

    wsi->begin_frame();

    if (image) {
        auto cmd = wsi->get_device().request_command_buffer();
        Image& swapchain_image = wsi->get_device().get_swapchain_view().get_image();
        VkOffset3D dst_extent = {int(swapchain_image.get_width()), int(swapchain_image.get_height()), 1};
        VkOffset3D src_extent = {int(image->get_width()),          int(image->get_height()),          1};

        cmd->image_barrier(swapchain_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

        Image& image_ref = image->get_view().get_image();

        cmd->image_barrier(image_ref, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);

        cmd->blit_image(swapchain_image, image_ref,
                        {}, dst_extent,
                        {}, src_extent,
                        0, 0);

        cmd->image_barrier(swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);

        cmd->uses_swapchain = true;
        wsi->get_device().submit(cmd);

    }
    wsi->end_frame();
    command_processor->begin_frame_context();
}

#define FROM_RDRAM(system, address) word_from_byte_array(system->mem.rdram, address)
#define FROM_DMEM(system, address) word_from_byte_array(system->mem.sp_dmem, address)

static const int command_lengths[64] = {
        2, 2, 2, 2, 2, 2, 2, 2, 8, 12, 24, 28, 24, 28, 40, 44,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 4, 4, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2
};

void process_commands_parallel_rdp(n64_system_t* system) {
    static int last_run_unprocessed_words = 0;

    n64_dpc_t* dpc = &system->dpc;

    // tell the game to not touch RDP stuff while we work
    dpc->status.freeze = true;

    // force align to 8 byte boundaries
    const word current = dpc->current & 0x00FFFFF8;
    const word end = dpc->end & 0x00FFFFF8;

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
            word command_word = FROM_DMEM(system, (current + i) & 0xFF8);
            parallel_rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    } else {
        if (end > 0x7FFFFFF || current > 0x7FFFFFF) {
            logwarn("Not running RDP commands, wanted to read past end of RDRAM!");
            return;
        }
        for (int i = 0; i < display_list_length; i += 4) {
            word command_word = FROM_RDRAM(system, current + i);
            parallel_rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    }

    int length_words = (display_list_length >> 2) + last_run_unprocessed_words;
    int buf_index = 0;

    bool processed_all = true;

    while (buf_index < length_words) {
        word command = (parallel_rdp_command_buffer[buf_index] >> 24) & 0x3F;

        int command_length = command_lengths[command];

        // Check we actually have enough bytes left in the display list for this command, and save the remainder of the display list for the next run, if not.
        if ((buf_index + command_length) * 4 > display_list_length + (last_run_unprocessed_words * 4)) {
            // Copy remaining bytes back to the beginning of the display list, and save them for next run.
            last_run_unprocessed_words = length_words - buf_index;

            // Safe to allocate this on the stack because we'll only have a partial command left, and that _has_ to be pretty small.
            word temp[last_run_unprocessed_words];
            for (int i = 0; i < last_run_unprocessed_words; i++) {
                temp[i] = parallel_rdp_command_buffer[buf_index + i];
            }

            for (int i = 0; i < last_run_unprocessed_words; i++) {
                parallel_rdp_command_buffer[i] = temp[i];
            }

            processed_all = false;

            break;
        }


        // Don't need to process commands under 8
        if (command >= 8) {
            command_processor->enqueue_command(command_length, &parallel_rdp_command_buffer[buf_index]);
        }

        if (RDP::Op(command) == RDP::Op::SyncFull) {
            command_processor->wait_for_timeline(command_processor->signal_timeline());
            interrupt_raise(INTERRUPT_DP);
        }

        buf_index += command_length;
    }

    if (processed_all) {
        last_run_unprocessed_words = 0;
    }

    dpc->current = end;

    dpc->status.freeze = false;
}
