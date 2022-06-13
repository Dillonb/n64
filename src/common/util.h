#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t half;
typedef uint32_t word;
typedef uint64_t dword;

typedef int8_t sbyte;
typedef int16_t shalf;
typedef int32_t sword;
typedef int64_t sdword;

#define se_32_64(val) ((sdword)((sword)(val)))

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
#include <linux/limits.h>
#else
#define PATH_MAX 0x1000
#endif

#endif
