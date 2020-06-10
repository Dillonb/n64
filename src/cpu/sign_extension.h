#ifndef N64_SIGN_EXTENSION_H
#define N64_SIGN_EXTENSION_H

#include "../common/util.h"

dword sign_extend_dword(dword v, int old, int new);
word sign_extend_word(word v, int old, int new);

#endif //N64_SIGN_EXTENSION_H
