#ifndef N64_QT_WSI_PLATFORM_H
#define N64_QT_WSI_PLATFORM_H

#undef signals
#include <rdp/parallel_rdp_wrapper.h>
#include <wsi.hpp>
#include <QVulkanInstance>

struct QtInstanceFactory : Vulkan::InstanceFactory {
    VkInstance create_instance(const VkInstanceCreateInfo *info) override {
        handle.setApiVersion({1, 3, 0});
        QByteArrayList exts;
        for (int i = 0; i < info->enabledExtensionCount; i++) {
            exts.push_back(QByteArray::fromStdString(info->ppEnabledExtensionNames[i]));
        }
        QByteArrayList layers;
        for (int i = 0; i < info->enabledLayerCount; i++) {
            layers.push_back(QByteArray::fromStdString(info->ppEnabledLayerNames[i]));
        }
        handle.setExtensions(exts);
        handle.setLayers(layers);
        handle.setApiVersion({1, 3, 0});
        handle.create();

        return handle.vkInstance();
    }

    QVulkanInstance handle;
};

class QtWSIPlatform : public Vulkan::WSIPlatform {
public:
    explicit QtWSIPlatform(QWindow* window);

    std::vector<const char *> get_instance_extensions() override;

    VkSurfaceKHR create_surface(VkInstance, VkPhysicalDevice) override;

    void destroy_surface(VkInstance, VkSurfaceKHR) override {}

    uint32_t get_surface_width() override;

    uint32_t get_surface_height() override;

    bool alive(Vulkan::WSI &wsi) override;

    void poll_input() override;

    void event_frame_tick(double frame, double elapsed) override;

    const VkApplicationInfo *get_application_info() override {
        return &appInfo;
    }

    void poll_input_async(Granite::InputTrackerHandler* handler) override {}

    QWindow* getWindowHandle() { return window; }

private:
    QWindow* window;

    VkApplicationInfo appInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_1
    };
};


#endif //N64_QT_WSI_PLATFORM_H
