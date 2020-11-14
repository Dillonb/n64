#include "parallel_rdp_wrapper.h"
#include <device.hpp>
#include <memory>
#include <SDL_video.h>
#include <SDL_vulkan.h>

using namespace Vulkan;

static std::unique_ptr<Device> vk_device;
static std::unique_ptr<Context> vk_context;

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
    logfatal("init_parallel_rdp");
}