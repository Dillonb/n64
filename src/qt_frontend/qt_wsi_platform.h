#ifndef N64_QT_WSI_PLATFORM_H
#define N64_QT_WSI_PLATFORM_H

#undef signals
#include <wsi.hpp>
#include <QVulkanInstance>
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

    const VkApplicationInfo *get_application_info() override {
        return &appInfo;
    }

private:
    void init_q_vulkan_instance(VkInstance instance);

    VulkanPane* vkPane;
    QVulkanInstance vkInstance;
    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_1
    };

};


#endif //N64_QT_WSI_PLATFORM_H
