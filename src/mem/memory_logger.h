#ifndef N64_MEMORY_LOGGER_H
#define N64_MEMORY_LOGGER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <r4300i.h>
#include <util.h>

typedef enum memory_access_size {
    MEMORY_ACCESS_SIZE_BYTE = 1,
    MEMORY_ACCESS_SIZE_HALF = 2,
    MEMORY_ACCESS_SIZE_WORD = 4,
    MEMORY_ACCESS_SIZE_DWORD = 8
} memory_access_size_t;

typedef struct memory_access {
    u32 paddr;
    bus_access_t access_type;
    memory_access_size_t size;
    u64 value;
} memory_access_t;

void init_memory_logger();
void log_memory_read(u32 paddr, memory_access_size_t size, u64 value);
void log_memory_write(u32 paddr, memory_access_size_t size, u64 value);

bool pop_memory_access(memory_access_t* access, memory_access_size_t size, bus_access_t access_type);
void clear_memory_logger();

#ifdef __cplusplus
}
#endif

#endif // N64_MEMORY_LOGGER_H
