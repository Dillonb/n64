#include <system/n64system.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <mem/pif.h>
#include <frontend/audio.h>
#include <frontend/render.h>
#include "n64_emulator_thread.h"
#include "qt_wsi_platform.h"

class QtParallelRdpWindowInfo : public ParallelRdpWindowInfo {
public:
    QtParallelRdpWindowInfo(QWindow* pane) : pane(pane) {}
    CoordinatePair get_window_size() override {
        return CoordinatePair {
            pane->width(),
            pane->height()
        };
    };
private:
    QWindow* pane;
};

N64EmulatorThread::N64EmulatorThread(QtWSIPlatform* wsiPlatform) {
    this->wsiPlatform = wsiPlatform;
    init_n64system("sm64.z64", true, false, QT_VULKAN_VIDEO_TYPE, false);

    if (file_exists(PIF_ROM_PATH)) {
        logalways("Found PIF ROM at %s, loading", PIF_ROM_PATH);
        load_pif_rom(PIF_ROM_PATH);
    }
    if (n64sys.mem.rom.rom != nullptr) {
        pif_rom_execute();
    }
}

void N64EmulatorThread::start() {
    if (n64_should_quit() || running) {
        logfatal("Tried to start emulator thread, but it was already running!");
    }

    QtWSIPlatform* _wsiPlatform = this->wsiPlatform;
    QWindow* pane = wsiPlatform->getPane();
    running = true;
    emuThread = std::thread([_wsiPlatform, pane]() {
        init_vulkan_wsi(_wsiPlatform, std::make_unique<QtParallelRdpWindowInfo>(pane));

        init_parallel_rdp();

        n64_system_loop();
    });
}

void N64EmulatorThread::reset() {
    if (running) {
        n64_queue_action(N64_ACTION_RESET);
    }
}
