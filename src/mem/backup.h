#ifndef N64_BACKUP_H
#define N64_BACKUP_H
#include <system/n64system.h>
#include "mem_util.h"

void backup_write_word(u32 index, u32 value);
u32 backup_read_word(u32 index);

void backup_write_byte(u32 index, u8 value);
u8 backup_read_byte(u32 index);

size_t get_save_size(n64_save_type_t save_type);
void init_savedata(n64_mem_t* mem, const char* rom_path);
void init_mempack(n64_mem_t* mem, const char* rom_path);

void persist_backup();
void force_persist_backup();

#endif //N64_BACKUP_H
