#include "crashdump.h"
#include <dynarec/rsp_dynarec.h>
#include <generated/version.h>
#include <rsp.h>

const char* n64_save_system_state() {
    n64_crashdump_t* crash_dump = malloc(sizeof(n64_crashdump_t));

    memcpy(&crash_dump->git_commit_hash, N64_GIT_COMMIT_HASH, sizeof(N64_GIT_COMMIT_HASH));
    memcpy(&crash_dump->system, &n64sys, sizeof(n64_system_t));


    crash_dump->system_size = sizeof(n64_system_t);
    crash_dump->system_base = (uintptr_t)&n64sys;
    memcpy(&crash_dump->system, &n64sys, crash_dump->system_size);

    crash_dump->cpu_size = sizeof(r4300i_t);
    crash_dump->cpu_base = (uintptr_t)n64cpu_ptr;
    memcpy(&crash_dump->cpu, n64cpu_ptr, crash_dump->cpu_size);

    crash_dump->rsp_size = sizeof(rsp_t);
    crash_dump->rsp_base = (uintptr_t)&n64rsp;
    memcpy(&crash_dump->rsp, &n64rsp, crash_dump->rsp_size);

    crash_dump->scheduler_size = sizeof(scheduler_t);
    crash_dump->scheduler_base = (uintptr_t)&n64scheduler;
    memcpy(&crash_dump->scheduler, &n64scheduler, crash_dump->scheduler_size);

    crash_dump->dynarec_size = sizeof(n64_dynarec_t);
    crash_dump->dynarec_base = (uintptr_t)&n64dynarec;
    memcpy(&crash_dump->dynarec, &n64dynarec, crash_dump->dynarec_size);

    crash_dump->rsp_dynarec_size = sizeof(rsp_dynarec_t);
    crash_dump->rsp_dynarec_base = (uintptr_t)n64rsp.dynarec;
    memcpy(&crash_dump->rsp_dynarec, n64rsp.dynarec, crash_dump->rsp_dynarec_size);

    crash_dump->codecache_size = CODECACHE_SIZE;
    crash_dump->codecache_base = (uintptr_t)n64dynarec.codecache;
    memcpy(&crash_dump->cpu_codecache, n64dynarec.codecache, crash_dump->codecache_size);

    crash_dump->rsp_codecache_size = RSP_CODECACHE_SIZE;
    crash_dump->rsp_codecache_base = (uintptr_t)n64rsp.dynarec->codecache;
    memcpy(&crash_dump->rsp_codecache, n64rsp.dynarec->codecache, crash_dump->rsp_codecache_size);

    size_t needed = snprintf(NULL, 0, "%s.crashdump", n64sys.rom_path);
    char* path_buf = malloc(needed + 1);
    snprintf(path_buf, needed + 1, "%s.crashdump", n64sys.rom_path);

    logalways("Saving to %s", path_buf);

    FILE* f = fopen(path_buf, "wbx");

    int suffix = 0;
    while (f == NULL) {
        logalways("Unable to create %s", path_buf);
        free(path_buf);
        needed = snprintf(NULL, 0, "%s.crashdump.%d", n64sys.rom_path, suffix);
        path_buf = malloc(needed + 1);
        snprintf(path_buf, needed + 1, "%s.crashdump.%d", n64sys.rom_path, suffix);

        logalways("Trying %s", path_buf);
        f = fopen(path_buf, "wbx");

        suffix++;
    }

    fwrite(crash_dump, sizeof(n64_crashdump_t), 1, f);
    logalways("Saved to %s", path_buf);
    return path_buf;
}