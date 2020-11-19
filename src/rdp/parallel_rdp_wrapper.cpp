#include "parallel_rdp_wrapper.h"
#include "rdp.h"
#include <memory>
#include <SDL_video.h>
#include <rdp_device.hpp>
#include <wsi.hpp>
#include <frontend/render.h>
#include <SDL_vulkan.h>

using namespace Vulkan;
using std::unique_ptr;
using RDP::CommandProcessor;
using RDP::CommandProcessorFlags;
using RDP::VIRegister;

static unique_ptr<CommandProcessor> command_processor;
std::vector<Semaphore> acquire_semaphore;

GFX_INFO parallel_rdp_gfx_info;

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


static unique_ptr<WSI> wsi;

void load_parallel_rdp(struct n64_system* system) {
    wsi = std::make_unique<WSI>();
    wsi->set_platform(new SDLWSIPlatform());
    if (!wsi->init(1)) {
        logfatal("Failed to initialize WSI!");
    }

    parallel_rdp_gfx_info = get_gfx_info(system);

    auto aligned_rdram = reinterpret_cast<uintptr_t>(parallel_rdp_gfx_info.RDRAM);
    uintptr_t offset = 0;

    if (wsi->get_device().get_device_features().supports_external_memory_host)
    {
        size_t align = wsi->get_device().get_device_features().host_memory_properties.minImportedHostPointerAlignment;
        offset = aligned_rdram & (align - 1);
        aligned_rdram -= offset;
    } else {
        logwarn("VK_EXT_external_memory_host is not supported by this device. Application might run slower because of this.");
    }

    CommandProcessorFlags flags = 0; // TODO setup scaling in here later

    command_processor = std::make_unique<CommandProcessor>(wsi->get_device(), reinterpret_cast<void *>(aligned_rdram),
                                        offset, 8 * 1024 * 1024, 4 * 1024 * 1024, flags);

    if (!command_processor->device_is_supported()) {
        logfatal("This device probably does not support 8/16-bit storage. Make sure you're using up-to-date drivers!");
    }
}

void update_screen_parallel_rdp() {
    if (!command_processor) {
        logfatal("Update screen without an initialized command processor");
    }


    wsi->begin_frame();

    command_processor->set_vi_register(VIRegister::Control,      *parallel_rdp_gfx_info.VI_STATUS_REG);
    command_processor->set_vi_register(VIRegister::Origin,       *parallel_rdp_gfx_info.VI_ORIGIN_REG);
    command_processor->set_vi_register(VIRegister::Width,        *parallel_rdp_gfx_info.VI_WIDTH_REG);
    command_processor->set_vi_register(VIRegister::Intr,         *parallel_rdp_gfx_info.VI_INTR_REG);
    command_processor->set_vi_register(VIRegister::VCurrentLine, *parallel_rdp_gfx_info.VI_V_CURRENT_LINE_REG);
    command_processor->set_vi_register(VIRegister::Timing,       *parallel_rdp_gfx_info.VI_V_BURST_REG);
    command_processor->set_vi_register(VIRegister::VSync,        *parallel_rdp_gfx_info.VI_V_SYNC_REG);
    command_processor->set_vi_register(VIRegister::HSync,        *parallel_rdp_gfx_info.VI_H_SYNC_REG);
    command_processor->set_vi_register(VIRegister::Leap,         *parallel_rdp_gfx_info.VI_LEAP_REG);
    command_processor->set_vi_register(VIRegister::HStart,       *parallel_rdp_gfx_info.VI_H_START_REG);
    command_processor->set_vi_register(VIRegister::VStart,       *parallel_rdp_gfx_info.VI_V_START_REG);
    command_processor->set_vi_register(VIRegister::VBurst,       *parallel_rdp_gfx_info.VI_V_BURST_REG);
    command_processor->set_vi_register(VIRegister::XScale,       *parallel_rdp_gfx_info.VI_X_SCALE_REG);
    command_processor->set_vi_register(VIRegister::YScale,       *parallel_rdp_gfx_info.VI_Y_SCALE_REG);

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

        cmd->blit_image(swapchain_image, image->get_view().get_image(),
                        {}, dst_extent,
                        {}, src_extent,
                        0, 0);

        cmd->uses_swapchain = true;
        wsi->get_device().submit(cmd);

    }
    wsi->end_frame();
    command_processor->begin_frame_context();
}