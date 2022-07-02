#ifndef N64_VULKAN_RENDERER_H
#define N64_VULKAN_RENDERER_H


#include <QVulkanWindowRenderer>


class VulkanRenderer : public QVulkanWindowRenderer {
public:
    VulkanRenderer();
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;
};


#endif //N64_VULKAN_RENDERER_H
