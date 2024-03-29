#ifndef N64_N64_EMULATOR_THREAD_H
#define N64_N64_EMULATOR_THREAD_H

#undef signals
#include <wsi.hpp>

class QtWSIPlatform;
class N64EmulatorThread {
public:
    explicit N64EmulatorThread(QtWSIPlatform* wsiPlatform);
    void start();
    void reset();
    void loadRom(const std::string& filename);

private:
    bool running = false;
    bool game_loaded = false;
    std::thread emuThread;
    QtWSIPlatform* wsiPlatform;
};


#endif //N64_N64_EMULATOR_THREAD_H
