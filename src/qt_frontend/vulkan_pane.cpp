#include <qt_frontend/vulkan_pane.h>
#include <QGuiApplication>
#include <log.h>

#include <QWindow>

#include "qt_wsi_platform.h"

enum class CompositorCategory { Windows, MacOS, XCB, Wayland };

static CompositorCategory GetOSCompositorCategory() {
    const QString platform_name = QGuiApplication::platformName();
    if (platform_name == QStringLiteral("windows"))
        return CompositorCategory::Windows;
    if (platform_name == QStringLiteral("xcb"))
        return CompositorCategory::XCB;
    if (platform_name == QStringLiteral("wayland") || platform_name == QStringLiteral("wayland-egl"))
        return CompositorCategory::Wayland;
    if (platform_name == QStringLiteral("cocoa") || platform_name == QStringLiteral("ios"))
        return CompositorCategory::MacOS;

    logwarn("Unknown Qt platform!");
    return CompositorCategory::Windows;
}

VulkanPane::VulkanPane() {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    if (GetOSCompositorCategory() == CompositorCategory::Wayland) {
        setAttribute(Qt::WA_DontCreateNativeAncestors);
    }

    if (GetOSCompositorCategory() == CompositorCategory::MacOS) {
        windowHandle()->setSurfaceType(QWindow::MetalSurface);
    } else {
        windowHandle()->setSurfaceType(QWindow::VulkanSurface);
    }

    if (!Vulkan::Context::init_loader(nullptr)) {
        logfatal("Could not initialize Vulkan ICD");
    }


    qtVkInstanceFactory = std::make_unique<QtInstanceFactory>();

    windowHandle()->setVulkanInstance(&qtVkInstanceFactory->handle);
    windowHandle()->create();

    platform = std::make_unique<QtWSIPlatform>(windowHandle());
    emulatorThread = std::make_unique<N64EmulatorThread>(qtVkInstanceFactory.get(), platform.get());
    emulatorThread->start();
}
