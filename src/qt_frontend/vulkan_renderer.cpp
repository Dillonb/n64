#include <log.h>
#include <rdp/parallel_rdp_wrapper.h>
#include "vulkan_renderer.h"
#include "qt_wsi_platform.h"

VulkanRenderer::VulkanRenderer(std::unique_ptr<QtWSIPlatform> wsiPlatform) {
    if (!wsiPlatform) {
        logfatal("VulkanRenderer created without a valid WSIPlatform instance");
    }
    platform = std::move(wsiPlatform);
}

void VulkanRenderer::startNextFrame() {
    printf("startNextFrame\n");
    fflush(stdout);
}

void VulkanRenderer::initResources() {
    printf("initResources\n");
    fflush(stdout);

    wsi = init_vulkan_wsi(platform.get(), false);
    init_parallel_rdp();

    auto context = std::make_unique<Vulkan::Context>();
    auto vkPane = platform->get_pane();
    context->init_from_instance_and_device(
            vkPane->vulkanInstance()->vkInstance(),
            vkPane->physicalDevice(),
            vkPane->device(),
            vkPane->graphicsQueue(),
            vkPane->graphicsQueueFamilyIndex());


    wsi->init_external_context(std::move(context));
}

void VulkanRenderer::initSwapChainResources() {
    printf("initSwapChainResources\n");
    fflush(stdout);
    auto vkPane = platform->get_pane();
    std::vector<Vulkan::ImageHandle> swapchain;

    for (int i = 0; i < vkPane->swapChainImageCount(); i++) {
        Vulkan::DeviceAllocation deviceAllocation;
        Vulkan::ImageCreateInfo imageCreateInfo;
        /*
        Vulkan::Image img(
                wsi->get_device(),
                vkPane->swapChainImage(i),
                vkPane->swapChainImageView(i),
                deviceAllocation,
                imageCreateInfo,
                VkImageViewType::VK_IMAGE_VIEW_TYPE_2D); // idk
                */
        //Vulkan::ImageHandle imgh(new Vulkan::Image())

        //swapchain.push_back(img)
    }
    emulatorThread = std::make_unique<N64EmulatorThread>(platform.get());
    emulatorThread->start();
}

void VulkanRenderer::releaseSwapChainResources() {
    printf("releaseSwapChainResources\n");
    fflush(stdout);
}

void VulkanRenderer::releaseResources() {
    printf("releaseResources\n");
    fflush(stdout);
}
