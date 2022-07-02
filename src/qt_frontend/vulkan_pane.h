#ifndef N64_VULKAN_PANE_H
#define N64_VULKAN_PANE_H


#include <QVulkanWindow>
#include "vulkan_renderer.h"

class VulkanPane : public QVulkanWindow {
public:
    explicit VulkanPane() : renderer(new VulkanRenderer()) {}

protected:
    QVulkanWindowRenderer * createRenderer() override;
private:
    VulkanRenderer* renderer = nullptr;
};


#endif //N64_VULKAN_PANE_H
