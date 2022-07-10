#ifndef N64_VULKAN_PANE_H
#define N64_VULKAN_PANE_H


#undef signals
#include <wsi.hpp>
#include <QWindow>
#include "n64_emulator_thread.h"

class VulkanPane : public QWindow {
public:
    explicit VulkanPane();

    void showEvent(QShowEvent* event) override;

private:
    std::unique_ptr<N64EmulatorThread> emulatorThread;
    std::unique_ptr<QtWSIPlatform> platform;
    Vulkan::WSI* wsi;
};


#endif //N64_VULKAN_PANE_H
