#include <system/n64system.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <mem/pif.h>
#include "n64_emulator_thread.h"
#include "qt_wsi_platform.h"

N64EmulatorThread::N64EmulatorThread(QtWSIPlatform* wsiPlatform) {
    init_n64system("sm64.z64", false, false, VULKAN_VIDEO_TYPE, false);

    if (file_exists(PIF_ROM_PATH)) {
        logalways("Found PIF ROM at %s, loading", PIF_ROM_PATH);
        load_pif_rom(PIF_ROM_PATH);
    }
    if (n64sys.mem.rom.rom != nullptr) {
        pif_rom_execute();
    }
}

void N64EmulatorThread::start() {
    if (n64_should_quit()) {
        logfatal("Tried to start emulator thread, but it was already running!");
    }

    emuThread = std::thread([]() {
        n64_system_loop();
    });
}
