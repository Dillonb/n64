#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define se_32_64(val) ((s64)((s32)(val)))

#define popcount(x) __builtin_popcountll(x)
#define FAKELITTLE_HALF(h) ((((h) >> 8u) & 0xFFu) | (((h) << 8u) & 0xFF00u))
#define FAKELITTLE_WORD(w) (FAKELITTLE_HALF((w) >> 16u) | (FAKELITTLE_HALF((w) & 0xFFFFu)) << 16u)

#define INLINE static inline __attribute__((always_inline))
#define PACKED __attribute__((__packed__))

#define unlikely(exp) __builtin_expect(exp, 0)
#define likely(exp) __builtin_expect(exp, 1)

#define N64_APP_NAME "dgb n64"

#ifdef N64_WIN
#define ASSERTWORD(type) _Static_assert(sizeof(type) == 4, "must be 32 bits")
#define ASSERTDWORD(type) _Static_assert(sizeof(type) == 8, "must be 64 bits")
#elif defined(__cplusplus)
#define ASSERTWORD(type) static_assert(sizeof(type) == 4, #type " must be 32 bits")
#define ASSERTDWORD(type) static_assert(sizeof(type) == 8, #type " must be 64 bits")
#else
#define ASSERTWORD(type) _Static_assert(sizeof(type) == 4, #type " must be 32 bits")
#define ASSERTDWORD(type) _Static_assert(sizeof(type) == 8, #type " must be 64 bits")
#endif

#ifndef N64_WIN
#include <unistd.h>
#include <linux/limits.h>
#else
#define PATH_MAX 0x1000
#endif


INLINE int clz32(u32 val) {
    return __builtin_clz(val);
}

INLINE u32 npow2(u32 x) {
    if (x <= 1) {
        return 1;
    }

    return 1u << (32 - __builtin_clz(x - 1));
}

INLINE bool file_exists(const char* path) {
#ifndef N64_WIN
    return access(path, F_OK) == 0;
#else
    FILE* f = fopen(path, "r");
    bool exists = false;
    if (f) {
        exists = true;
        fclose(f);
    }
    return exists;
#endif
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif
