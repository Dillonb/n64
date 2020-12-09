#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdint.h>

#ifdef N64_WIN
#include <Windows.h>
#undef min
#undef max
#else
#define byte uint8_t
#endif
#define half uint16_t
#define word uint32_t
#define dword uint64_t

#define sbyte int8_t
#define shalf int16_t
#define sword int32_t
#define sdword int64_t

#define PRINTF_BYTE "0x%02X"
#define PRINTF_HALF "0x%04X"
#define PRINTF_WORD "0x%08X"
#define PRINTF_DWORD "0x%016X"

#define popcount(x) __builtin_popcountll(x)
#define FAKELITTLE_HALF(h) ((((h) >> 8u) & 0xFFu) | (((h) << 8u) & 0xFF00u))
#define FAKELITTLE_WORD(w) (FAKELITTLE_HALF((w) >> 16u) | (FAKELITTLE_HALF((w) & 0xFFFFu)) << 16u)

#define INLINE static inline __attribute__((always_inline))

#define unlikely(exp) __builtin_expect(exp, 0)
#define likely(exp) __builtin_expect(exp, 1)

#define N64_APP_NAME "dgb n64"

#endif
