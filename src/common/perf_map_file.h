#ifndef N64_PERF_MAP_FILE_H
#define N64_PERF_MAP_FILE_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __linux__
void n64_perf_map_file_write(const uintptr_t address, size_t code_size, const char* name);
void n64_perf_map_file_enable();
#else
#define n64_perf_map_file_write(address, code_size, name) do {} while (0)
#endif

#endif // N64_PERF_MAP_FILE_H
