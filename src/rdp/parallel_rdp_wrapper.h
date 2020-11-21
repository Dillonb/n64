#ifndef N64_PARALLEL_RDP_WRAPPER_H
#define N64_PARALLEL_RDP_WRAPPER_H

#include <system/n64system.h>

#ifdef __cplusplus
extern "C" {
#endif
    void load_parallel_rdp(n64_system_t* system);
    void update_screen_parallel_rdp(n64_system_t* system);
#ifdef __cplusplus
};
#endif

#endif //N64_PARALLEL_RDP_WRAPPER_H
