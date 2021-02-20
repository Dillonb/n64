#include <stdio.h>
#include "softrdp.h"

typedef uint8_t byte;
typedef uint16_t half;
typedef uint32_t word;
typedef uint64_t dword;

typedef int8_t sbyte;
typedef int16_t shalf;
typedef int32_t sword;
typedef int64_t sdword;

void init_softrdp() {

}

void enqueue_command_softrdp(int command_length, uint32_t* buffer) {
    for (int i = 0; i < command_length; i++) {
        printf("Ignoring RDP command word: 0x%08X\n", buffer[i]);
    }
}
