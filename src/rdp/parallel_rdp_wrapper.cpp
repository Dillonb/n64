#include "parallel_rdp_wrapper.h"
#include "rdp.h"
#include <device.hpp>
#include <memory>
#include <SDL_video.h>
#include <SDL_vulkan.h>
#include <rdp_device.hpp>
#include <frontend/render.h>

using namespace Vulkan;
using std::unique_ptr;
using RDP::CommandProcessor;
using RDP::CommandProcessorFlags;
using RDP::VIRegister;

static unique_ptr<Device> vk_device;
static unique_ptr<Context> vk_context;
static unique_ptr<CommandProcessor> command_processor;

GFX_INFO parallel_rdp_gfx_info;

extern VkInstance vk_instance;
extern VkSurfaceKHR vk_surface;

extern const char* required_device_extensions[64];
extern uint32_t num_required_extensions;

extern const char* required_device_layers[64];
extern uint32_t num_required_device_layers;

extern "C" {
    extern SDL_Window* window;
}


void load_parallel_rdp(struct n64_system* system) {
    vk_context = std::make_unique<Context>();

    vk_context->init_loader(nullptr);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &num_required_extensions, required_device_extensions)) {
        logfatal("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
    }

    for (uint32_t i = 0; i < num_required_extensions; i++) {
        printf("Extension: %s\n", required_device_extensions[i]);
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pEngineName = N64_APP_NAME;
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pApplicationName = N64_APP_NAME;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &app_info;
    createInfo.enabledExtensionCount = num_required_extensions;
    createInfo.ppEnabledExtensionNames = required_device_extensions;

    if (vkCreateInstance(&createInfo, NULL, &vk_instance) != VK_SUCCESS) {
        logfatal("Failed to create Vulkan instance.");
    }

    volkLoadInstance(vk_instance);

    if (!SDL_Vulkan_CreateSurface(window, vk_instance, &vk_surface)) {

        logfatal("Failed to create Vulkan window surface: %s", SDL_GetError());
    }

    num_required_device_layers = 0;

    const VkPhysicalDeviceFeatures features = { 0 };

    vk_context->init_device_from_instance(vk_instance, nullptr, vk_surface, required_device_extensions,
                                          0, required_device_layers, num_required_device_layers,
                                          &features, CONTEXT_CREATION_DISABLE_BINDLESS_BIT);

    vk_device = std::make_unique<Device>();
    vk_device->set_context(*vk_context);

    unsigned mask = 1;
    unsigned num_sync_frames = 0;
    for (unsigned i = 0; i < 32; i++) {
        if (mask & (1u << i)) {
            num_sync_frames++;
        }
    }

    printf("num sync frames: %d\n", num_sync_frames);

    vk_device->init_frame_contexts(num_sync_frames);

    parallel_rdp_gfx_info = get_gfx_info(system);

    auto aligned_rdram = reinterpret_cast<uintptr_t>(parallel_rdp_gfx_info.RDRAM);
    uintptr_t offset = 0;

    if (vk_device->get_device_features().supports_external_memory_host)
    {
        size_t align = vk_device->get_device_features().host_memory_properties.minImportedHostPointerAlignment;
        offset = aligned_rdram & (align - 1);
        aligned_rdram -= offset;
    } else {
        logwarn("VK_EXT_external_memory_host is not supported by this device. Application might run slower because of this.");
    }

    CommandProcessorFlags flags = 0; // TODO setup scaling in here later

    command_processor = std::make_unique<CommandProcessor>(*vk_device, reinterpret_cast<void *>(aligned_rdram),
                                        offset, 8 * 1024 * 1024, 4 * 1024 * 1024, flags);

    if (!command_processor->device_is_supported()) {
        logfatal("This device probably does not support 8/16-bit storage. Make sure you're using up-to-date drivers!");
    }
}

void update_screen_parallel_rdp() {
    if (!command_processor) {
        logfatal("Update screen without an initialized command processor");
    }

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
    auto image = command_processor->scanout(opts);

    if (image) {
        printf("image is true\n");
        logfatal("display VkImage");
    } else {
        printf("image is false\n");
    }

    //unsigned index = vulkan->get_sync_index(vulkan->handle);

    //logfatal("parallel rdp update screen");
}