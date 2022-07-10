#include <frontend/render.h>
#include <frontend/frontend.h>
#include "qt_wsi_platform.h"

std::vector<const char *> QtWSIPlatform::get_instance_extensions() {
    auto vec = std::vector<const char*>();
    for (const auto &ext: vkInstance.supportedExtensions()) {
        vec.push_back(ext.name.data());
    }

    return vec;
}

VkSurfaceKHR QtWSIPlatform::create_surface(VkInstance instance, VkPhysicalDevice gpu) {
    init_q_vulkan_instance(instance);
    auto surface = vkInstance.surfaceForWindow(vkPane);
    if (!surface) {
        logfatal("Failed to create surface!");
    }
    return surface;
}

uint32_t QtWSIPlatform::get_surface_width() {
    return N64_SCREEN_X * SCREEN_SCALE;
}

uint32_t QtWSIPlatform::get_surface_height() {
    return N64_SCREEN_Y * SCREEN_SCALE;
}

QtWSIPlatform::QtWSIPlatform(VulkanPane *vkPane)
        : vkPane(vkPane) {

}

bool QtWSIPlatform::alive(Vulkan::WSI &wsi) {
    return true;
}

void QtWSIPlatform::poll_input() {
    n64_poll_input();
}

void QtWSIPlatform::event_frame_tick(double frame, double elapsed) {
    //n64_render_screen();
}

void QtWSIPlatform::init_q_vulkan_instance(VkInstance instance) {
    vkInstance.setVkInstance(instance);
#ifdef VULKAN_DEBUG
    vkInstance.setLayers({"VK_LAYER_KHRONOS_validation"});
#endif
    if (!vkInstance.create()) {
        logfatal("Failed to create vulkan instance! %d", vkInstance.errorCode());
    }
    vkPane->setVulkanInstance(&vkInstance);
}

