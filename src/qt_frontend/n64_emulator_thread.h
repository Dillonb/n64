#ifndef N64_N64_EMULATOR_THREAD_H
#define N64_N64_EMULATOR_THREAD_H

#undef signals
#include <wsi.hpp>

class QtWSIPlatform;
class N64EmulatorThread {
public:
    explicit N64EmulatorThread(QtWSIPlatform* wsiPlatform);
    void start();
private:
    std::thread emuThread;

};


#endif //N64_N64_EMULATOR_THREAD_H
