#ifndef N64_QT_WSI_PLATFORM_H
#define N64_QT_WSI_PLATFORM_H

#undef signals
#include <wsi.hpp>
#include "vulkan_pane.h"

class QtWSIPlatform : public Vulkan::WSIPlatform {
public:
    explicit QtWSIPlatform(VulkanPane* vkPane);

    std::vector<const char *> get_instance_extensions() override;

    VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice gpu) override;

    uint32_t get_surface_width() override;

    uint32_t get_surface_height() override;

    bool alive(Vulkan::WSI &wsi) override;

    void poll_input() override;

    void event_frame_tick(double frame, double elapsed) override;

    VulkanPane* get_pane() {
        return vkPane;
    }
private:
    VulkanPane* vkPane;
};


#endif //N64_QT_WSI_PLATFORM_H
