#ifndef N64_VULKAN_RENDERER_H
#define N64_VULKAN_RENDERER_H


#include <QVulkanWindowRenderer>
#include "n64_emulator_thread.h"


class QtWSIPlatform;
class VulkanRenderer : public QVulkanWindowRenderer {
public:
    VulkanRenderer(std::unique_ptr<QtWSIPlatform> wsiPlatform);
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;
private:
    std::unique_ptr<QtWSIPlatform> platform;
    std::unique_ptr<N64EmulatorThread> emulatorThread = nullptr;
    Vulkan::WSI* wsi;
};


#endif //N64_VULKAN_RENDERER_H
