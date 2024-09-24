#ifndef N64_VULKAN_PANE_H
#define N64_VULKAN_PANE_H

#undef signals
#include <rdp/parallel_rdp_wrapper.h>
#include <qt_frontend/n64_emulator_thread.h>
#include <qt_frontend/qt_wsi_platform.h>
#include <QVulkanWindow>
#include <QWidget>

class VulkanPane : public QWidget {
public:
    explicit VulkanPane();

    N64EmulatorThread& getEmulatorThread() { return *emulatorThread; }

private:
    std::unique_ptr<N64EmulatorThread> emulatorThread;
    std::unique_ptr<QtWSIPlatform> platform;
    std::unique_ptr<QtInstanceFactory> qtVkInstanceFactory;
};


#endif //N64_VULKAN_PANE_H
