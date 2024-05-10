#ifdef __linux__
#include "perf_map_file.h"
#include <inttypes.h>
#include <log.h>
#include <stdbool.h>
#include <linux/limits.h>
#include <stdio.h>
#include <unistd.h>

static bool perf_map_enabled = false;

FILE* perf_map_file_handle() {
    static FILE* perf_map_file = NULL;
    if (!perf_map_file) {
        char filename[PATH_MAX];
        snprintf(filename, PATH_MAX, "/tmp/perf-%d.map", getpid());
        perf_map_file = fopen(filename, "w");
        if (!perf_map_file) {
            logfatal("Failed to open perf map file");
        }
    }
    return perf_map_file;
}

void n64_perf_map_file_write(const uintptr_t address, size_t code_size, const char* name) {
    if (perf_map_enabled) {
        FILE* f = perf_map_file_handle();

        fprintf(f, "%" PRIx64 " %" PRIx64 " %s\n", address, code_size, name);
        printf("%" PRIx64 " %" PRIx64 " %s\n", address, code_size, name);
    }
}

void n64_perf_map_file_enable() {

}

#endif