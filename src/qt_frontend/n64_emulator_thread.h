#ifndef N64_N64_EMULATOR_THREAD_H
#define N64_N64_EMULATOR_THREAD_H

#undef signals
#include <wsi.hpp>
#include <QThread>

class QtWSIPlatform;
class N64EmulatorThread : public QThread {
    Q_OBJECT
    bool running = false;
    bool game_loaded = false;
    std::thread emuThread;
    QtWSIPlatform* wsiPlatform;
    Vulkan::InstanceFactory* instanceFactory;
public:
    explicit N64EmulatorThread(Vulkan::InstanceFactory* instanceFactory, QtWSIPlatform* wsiPlatform, const char* rom_path = nullptr, bool debug = false, bool interpreter = false);
    void run() noexcept override;
    void reset();
    void loadRom(const std::string& filename);
};


#endif //N64_N64_EMULATOR_THREAD_H
