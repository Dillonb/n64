#include "parallel_rdp_wrapper.h"

#include <device.hpp>
#include <memory>
#include <frontend/render.h>

static std::unique_ptr<Vulkan::Device> vk_device;
static std::unique_ptr<Vulkan::Context> vk_context;

extern VkInstance vk_instance;
extern VkPhysicalDevice vk_physical_device;
extern VkSurfaceKHR vk_surface;

extern const char* required_device_extensions[64];
extern uint32_t num_required_extensions;

extern const char* required_device_layers[64];
extern uint32_t num_required_device_layers;

extern const VkPhysicalDeviceFeatures* required_features;


void load_parallel_rdp(struct n64_system* system) {
    vk_context = std::make_unique<Vulkan::Context>();
    vk_context->init_device_from_instance(vk_instance, vk_physical_device, vk_surface, required_device_extensions,
                                          num_required_extensions, required_device_layers, num_required_device_layers,
                                          required_features, Vulkan::CONTEXT_CREATION_DISABLE_BINDLESS_BIT);
    vk_device = std::make_unique<Vulkan::Device>();
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