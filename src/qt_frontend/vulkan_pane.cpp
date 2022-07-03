#include "vulkan_pane.h"
#include "qt_wsi_platform.h"

QVulkanWindowRenderer* VulkanPane::createRenderer() {
    return renderer;
}

VulkanPane::VulkanPane(QVulkanInstance* vkInstance) {
    setVulkanInstance(vkInstance);
    renderer = new VulkanRenderer(std::make_unique<QtWSIPlatform>(this));
}
