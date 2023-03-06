#ifndef MPROTECT_UTILS_H
#define MPROTECT_UTILS_H
#include <util.h>
void mprotect_rwx(u8* cache, size_t size, const char* cache_name);
#endif // MPROTECT_UTILS_H
