#include <frontend/render.h>
#include <frontend/frontend.h>
#include <QWindow>
#include "qt_wsi_platform.h"

std::vector<const char *> QtWSIPlatform::get_instance_extensions() {
    auto vec = std::vector<const char*>();
    for (const auto &ext: window->vulkanInstance()->supportedExtensions()) {
        vec.push_back(ext.name.data());
    }

    return vec;
}

VkSurfaceKHR QtWSIPlatform::create_surface(VkInstance, VkPhysicalDevice) {
    return QVulkanInstance::surfaceForWindow(window);
}

uint32_t QtWSIPlatform::get_surface_width() {
    return N64_SCREEN_X * SCREEN_SCALE;
}

uint32_t QtWSIPlatform::get_surface_height() {
    return N64_SCREEN_Y * SCREEN_SCALE;
}

QtWSIPlatform::QtWSIPlatform(QWindow *window)
        : window(window) {

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
