#include <log.h>
#include <rdp/parallel_rdp_wrapper.h>
#include "vulkan_pane.h"
#include "qt_wsi_platform.h"

VulkanPane::VulkanPane() {
    setSurfaceType(QWindow::VulkanSurface);
}

void VulkanPane::showEvent(QShowEvent *event) {
    QWindow::showEvent(event);
    if (volkInitialize() != VK_SUCCESS) {
        logfatal("Failed to load Volk");
    }

    platform = std::make_unique<QtWSIPlatform>(this);

    emulatorThread = std::make_unique<N64EmulatorThread>(platform.get());
    emulatorThread->start();

}
