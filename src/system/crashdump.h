#ifndef N64_CRASHDUMP_H
#define N64_CRASHDUMP_H

#ifdef N64_DYNAREC_ENABLED
#include <dynarec/dynarec.h>
#endif
#include <system/n64system.h>
#include <system/scheduler.h>
#include <generated/version.h>

// 41 bytes = commit hash + null terminator
// 47 bytes = commit hash+ "-dirty" + null terminator
static_assert(sizeof(N64_GIT_COMMIT_HASH) == 41 || sizeof(N64_GIT_COMMIT_HASH) == 47, "git commit hash is not 41 or 47 bytes");
static_assert(sizeof(uintptr_t) == sizeof(u64), "uintptr_t should be 64 bit");
typedef struct n64_crashdump {
    char git_commit_hash[41];
    char crash_reason[1024]; // the logfatal message that made us crash

    size_t system_size;
    uintptr_t system_base;
    n64_system_t system;

    size_t cpu_size;
    uintptr_t cpu_base;
    r4300i_t cpu;

    size_t rsp_size;
    uintptr_t rsp_base;
    rsp_t rsp;

    size_t scheduler_size;
    uintptr_t scheduler_base;
    scheduler_t scheduler;

#ifdef N64_DYNAREC_ENABLED
    size_t dynarec_size;
    uintptr_t dynarec_base;
    n64_dynarec_t dynarec;

#ifdef N64_DYNAREC_V1_ENABLED
    size_t rsp_dynarec_size;
    uintptr_t rsp_dynarec_base;
    rsp_dynarec_t rsp_dynarec;
#endif

    size_t codecache_size;
    uintptr_t codecache_base;
    u8 cpu_codecache[CODECACHE_SIZE];

    size_t rsp_codecache_size;
    uintptr_t rsp_codecache_base;
    u8 rsp_codecache[RSP_CODECACHE_SIZE];
#endif
} n64_crashdump_t;


// Saves a crash dump to a file using the above structure. Takes the crash reason as a parameter.
// Saves it to <rom path>.crashdump (will append .N if the file exists, where N is a number)
// Returns the filename saved to
const char* n64_save_system_state(const char* crash_reason);

#endif // N64_CRASHDUMP_H