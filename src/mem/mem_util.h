#ifndef N64_MEM_UTIL_H
#define N64_MEM_UTIL_H

#include <util.h>
#include <string.h>
#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(N64_MACOS)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)
#define bswap_16(x) OSSwapInt16(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <sys/types.h>
#include <machine/bswap.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

#ifdef N64_BIG_ENDIAN
// No need to bswap anything on a big endian system
#ifndef be64toh
#define be64toh(x) x
#endif
#ifndef be32toh
#define be32toh(x) x
#endif
#ifndef be16toh
#define be16toh(x) x
#endif
#ifndef htobe64
#define htobe64(x) x
#endif
#ifndef htobe32
#define htobe32(x) x
#endif
#ifndef htobe16
#define htobe16(x) x
#endif
#else
#ifndef be64toh
#define be64toh(x) bswap_64(x)
#endif
#ifndef be32toh
#define be32toh(x) bswap_32(x)
#endif
#ifndef be16toh
#define be16toh(x) bswap_16(x)
#endif
#ifndef htobe64
#define htobe64(x) bswap_64(x)
#endif
#ifndef htobe32
#define htobe32(x) bswap_32(x)
#endif
#ifndef htobe16
#define htobe16(x) bswap_16(x)
#endif
#endif

#ifdef N64_BIG_ENDIAN
#define DWORD_ADDRESS(addr) (addr)
#define WORD_ADDRESS(addr) (addr)
#define HALF_ADDRESS(addr) (addr)
#define BYTE_ADDRESS(addr) (addr)
#else
#define DWORD_ADDRESS(addr) (addr)
#define WORD_ADDRESS(addr) (addr)
#define HALF_ADDRESS(addr) ((addr) ^ 2)
#define BYTE_ADDRESS(addr) ((addr) ^ 3)
#endif


INLINE dword dword_from_byte_array(byte* arr, word index) {
#ifdef N64_BIG_ENDIAN
    dword d;
    memcpy(&d, arr + index, sizeof(dword));
    return d;
#else
    word hi;
    memcpy(&hi, arr + index, sizeof(word));

    word lo;
    memcpy(&lo, arr + index + sizeof(word), sizeof(word));

    dword d = ((dword)hi << 32) | lo;
    return d;
#endif
}

INLINE word word_from_byte_array(byte* arr, word index) {
    word val;
    memcpy(&val, arr + index, sizeof(word));
    return val;
}

INLINE half half_from_byte_array(byte* arr, word index) {
    half h;
    memcpy(&h, arr + index, sizeof(half));
    return h;
}

INLINE void dword_to_byte_array(byte* arr, word index, dword value) {
#ifdef N64_BIG_ENDIAN
    memcpy(arr + index, &value, sizeof(dword));
#else
    word lo = value & 0xFFFFFFFF;
    value >>= 32;
    word hi = value & 0xFFFFFFFF;

    memcpy(arr + index, &hi, sizeof(word));
    memcpy(arr + index + sizeof(word), &lo, sizeof(word));
#endif
}

INLINE void word_to_byte_array(byte* arr, word index, word value) {
    memcpy(arr + index, &value, sizeof(word));
}

INLINE void half_to_byte_array(byte* arr, word index, half value) {
    memcpy(arr + index, &value, sizeof(half));
}

#endif //N64_MEM_UTIL_H
