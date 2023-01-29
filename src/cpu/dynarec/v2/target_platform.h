#ifndef N64_TARGET_PLATFORM_H
#define N64_TARGET_PLATFORM_H

const int* get_preserved_registers();
int get_num_preserved_registers();

const int* get_scratch_registers();
int get_num_scratch_registers();

#endif //N64_TARGET_PLATFORM_H
