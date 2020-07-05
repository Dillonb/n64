#ifndef N64_MEM_UTIL_H
#define N64_MEM_UTIL_H

#include <zconf.h>
#include "../common/util.h"

INLINE dword dword_from_byte_array(byte* arr, word index) {
    dword* dwarr = (dword*)arr;
    return be64toh(dwarr[index / sizeof(dword)]);
}

INLINE word word_from_byte_array(byte* arr, word index) {
    word* warr = (word*)arr;
    return be32toh(warr[index / sizeof(word)]);
}

INLINE half half_from_byte_array(byte* arr, word index) {
    half* warr = (half*)arr;
    return be16toh(warr[index / sizeof(half)]);
}

INLINE void dword_to_byte_array(byte* arr, word index, dword value) {
    dword* dwarr = (dword*)arr;
    dwarr[index / sizeof(dword)] = htobe64(value);
}

INLINE void word_to_byte_array(byte* arr, word index, word value) {
    word* warr = (word*)arr;
    warr[index / sizeof(word)] = htobe32(value);
}

INLINE void half_to_byte_array(byte* arr, word index, half value) {
    half* warr = (half*)arr;
    warr[index / sizeof(half)] = htobe16(value);
}

#endif //N64_MEM_UTIL_H
