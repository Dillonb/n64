#ifndef N64_MEM_UTIL_H
#define N64_MEM_UTIL_H

#include <string.h>

#include "util.h"
#include "log.h"

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
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

INLINE size_t safe_cart_byte_index(u32 addr, size_t rom_size) {
    u32 index = BYTE_ADDRESS(addr & 0xFFFFFFF);
    if (unlikely(index > rom_size)) {
        logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", addr, index, index);
    }
    return index;
}

#define RDRAM_BYTE(addr) n64sys.mem.rdram[(BYTE_ADDRESS(addr) & (N64_RDRAM_SIZE - 1))]
#define RDRAM_WORD(addr) ((u32*)n64sys.mem.rdram)[(WORD_ADDRESS(addr) & (N64_RDRAM_SIZE - 1)) >> 2]
#define CART_BYTE(addr, rom_size) n64sys.mem.rom.rom[safe_cart_byte_index(addr, rom_size)]

INLINE u64 dword_from_byte_array(u8* arr, u32 index) {
#ifdef N64_BIG_ENDIAN
    u64 d;
    memcpy(&d, arr + index, sizeof(u64));
    return d;
#else
    u32 hi;
    memcpy(&hi, arr + index, sizeof(u32));

    u32 lo;
    memcpy(&lo, arr + index + sizeof(u32), sizeof(u32));

    u64 d = ((u64)hi << 32) | lo;
    return d;
#endif
}

INLINE u32 word_from_byte_array(u8* arr, u32 index) {
    u32 val;
    memcpy(&val, arr + index, sizeof(u32));
    return val;
}

INLINE u16 half_from_byte_array(u8* arr, u32 index) {
    u16 h;
    memcpy(&h, arr + index, sizeof(u16));
    return h;
}

INLINE void dword_to_byte_array(u8* arr, u32 index, u64 value) {
#ifdef N64_BIG_ENDIAN
    memcpy(arr + index, &value, sizeof(u64));
#else
    u32 lo = value & 0xFFFFFFFF;
    value >>= 32;
    u32 hi = value & 0xFFFFFFFF;

    memcpy(arr + index, &hi, sizeof(u32));
    memcpy(arr + index + sizeof(u32), &lo, sizeof(u32));
#endif
}

INLINE void word_to_byte_array(u8* arr, u32 index, u32 value) {
    memcpy(arr + index, &value, sizeof(u32));
}

INLINE void half_to_byte_array(u8* arr, u32 index, u16 value) {
    memcpy(arr + index, &value, sizeof(u16));
}

#endif //N64_MEM_UTIL_H
