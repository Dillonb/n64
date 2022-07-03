#include <frontend/render.h>
#include "qt_wsi_platform.h"

std::vector<const char *> QtWSIPlatform::get_instance_extensions() {
    auto vec = std::vector<const char*>();
    auto instance = vkPane->vulkanInstance();
    for (const auto &ext: instance->extensions()) {
        vec.push_back(ext.data());
    }

    return vec;
}

VkSurfaceKHR QtWSIPlatform::create_surface(VkInstance instance, VkPhysicalDevice gpu) {
    auto surface = vkPane->vulkanInstance()->surfaceForWindow(vkPane);
    if (!surface) {
        logfatal("Failed to create surface!");
    }
    printf("Created surface\n");
    return surface;
    /*
    VkSurfaceKHR vk_surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, &vk_surface)) {
        logfatal("Failed to create Vulkan window surface: %s", SDL_GetError());
    }
    return vk_surface;
     */
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
    //n64_poll_input();
}

void QtWSIPlatform::event_frame_tick(double frame, double elapsed) {
    //n64_render_screen();
}

