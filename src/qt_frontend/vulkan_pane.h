#ifndef N64_VULKAN_PANE_H
#define N64_VULKAN_PANE_H


#undef signals
#include <wsi.hpp>
#include <QVulkanWindow>
#include "vulkan_renderer.h"

class VulkanPane : public QVulkanWindow {
public:
    explicit VulkanPane(QVulkanInstance* vkInstance);

protected:
    QVulkanWindowRenderer * createRenderer() override;
private:
    VulkanRenderer* renderer = nullptr;
};


#endif //N64_VULKAN_PANE_H
