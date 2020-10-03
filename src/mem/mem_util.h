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


INLINE dword dword_from_byte_array(byte* arr, word index) {
    dword* dwarr = (dword*)arr;
    return be64toh(dwarr[index / sizeof(dword)]);
}

INLINE word word_from_byte_array(byte* arr, word index) {
    word val;
    memcpy(&val, (word*)(arr + index), sizeof(word));
    return be32toh(val);
}

INLINE word word_from_byte_array_unaligned(byte* arr, word index) {
    word* warr = (word*)(arr + index);
    return be32toh(warr[0]);
}

INLINE half half_from_byte_array(byte* arr, word index) {
    half* warr = (half*)arr;
    return be16toh(warr[index / sizeof(half)]);
}

INLINE half half_from_byte_array_unaligned(byte* arr, word index) {
    half* harr = (half*)(arr + index);
    return be16toh(harr[0]);
}

INLINE void dword_to_byte_array(byte* arr, word index, dword value) {
    dword* dwarr = (dword*)arr;
    dwarr[index / sizeof(dword)] = htobe64(value);
}

INLINE void word_to_byte_array(byte* arr, word index, word value) {
    word* warr = (word*)arr;
    warr[index / sizeof(word)] = htobe32(value);
}

INLINE void word_to_byte_array_unaligned(byte* arr, word index, word value) {
    word* warr = (word*)(arr + index);
    warr[0] = htobe32(value);
}

INLINE void half_to_byte_array(byte* arr, word index, half value) {
    half* warr = (half*)arr;
    warr[index / sizeof(half)] = htobe16(value);
}

INLINE void half_to_byte_array_unaligned(byte* arr, word index, half value) {
    half* warr = (half*)(arr + index);
    warr[0] = htobe16(value);
}

#endif //N64_MEM_UTIL_H
